// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "timelinemodel.h"
#include "timelinemodel_p.h"
#include "timelinemodelaggregator.h"
#include "timelineitemsrenderpass.h"
#include "timelineselectionrenderpass.h"
#include "timelinenotesrenderpass.h"

#include <utils/qtcassert.h>

#include <list>

namespace Timeline {

/*!
    \class Timeline::TimelineModel
    \brief The TimelineModel class provides a sorted model for timeline data.

    The TimelineModel lets you keep range data sorted by both start and end times, so that
    visible ranges can easily be computed. The only precondition for that to work is that the ranges
    must be perfectly nested. A "parent" range of a range R is defined as a range for which the
    start time is earlier than R's start time and the end time is later than R's end time. A set
    of ranges is perfectly nested if all parent ranges of any given range have a common parent
    range. Mind that you can always make that happen by defining a range that spans the whole
    available time span. That, however, will make any code that uses firstIndex() and lastIndex()
    for selecting subsets of the model always select all of it.

    \note Indices returned from the various methods are only valid until a new range is inserted
          before them. Inserting a new range before a given index moves the range pointed to by the
          index by one. Incrementing the index by one will make it point to the item again.
*/

/*!
    Compute all ranges' parents.
    \sa firstIndex()
*/
void TimelineModel::computeNesting()
{
    std::list<int> parents;
    for (int range = 0; range != count(); ++range) {
        TimelineModelPrivate::Range &current = d->ranges[range];
        for (std::list<int>::iterator parentIt = parents.begin();;) {
            if (parentIt == parents.end()) {
                parents.push_back(range);
                break;
            }

            TimelineModelPrivate::Range &parent = d->ranges[*parentIt];
            qint64 parentEnd = parent.start + parent.duration;
            if (parentEnd < current.start) {
                // We've completely passed the parent. Remove it.
                parentIt = parents.erase(parentIt);
            } else if (parentEnd >= current.start + current.duration) {
                // Current range is completely inside the parent range: no need to insert
                current.parent = (parent.parent == -1 ? *parentIt : parent.parent);
                break;
            } else if (parent.start == current.start) {
                // The parent range starts at the same time but ends before the current range.
                // We could switch them but that would violate the order requirements. When
                // searching for ranges between two timestamps we'd skip the ranges between the
                // current range and the parent range if the start timestamp points into the parent
                // range. firstIndex() would then return the current range, which has an id greater
                // than the parent. The parent could not be found then. To deal with this corner
                // case, we assign the parent the "wrong" way around, so that on firstIndex() we
                // always end up with the smallest id of any ranges starting at the same time.

                // The other way to deal with this would be fixing up the ordering on insert. In
                // fact we do that on insertStart().
                // However, in order to rely on this we would also have to move the start index if
                // on insertEnd() it turns out that the range just being ended is shorter than a
                // previous one starting at the same time. We don't want to do that as client code
                // could not find out about the changes in the IDs for range starts then.

                current.parent = *parentIt;
                parents.push_back(range);
                break;
            } else {
                ++parentIt;
            }
        }
    }
}

int TimelineModel::collapsedRowCount() const
{
    return d->collapsedRowCount;
}

void TimelineModel::setCollapsedRowCount(int rows)
{
    if (d->collapsedRowCount != rows)
        d->collapsedRowCount = rows;
}

int TimelineModel::expandedRowCount() const
{
    return d->expandedRowCount;
}

void TimelineModel::setExpandedRowCount(int rows)
{
    if (d->expandedRowCount != rows) {
        if (d->rowOffsets.length() > rows)
            d->rowOffsets.resize(rows);
        d->expandedRowCount = rows;
    }
}

int TimelineModel::row(int index) const
{
    return expanded() ? expandedRow(index) : collapsedRow(index);
}

TimelineModel::TimelineModelPrivate::TimelineModelPrivate(int modelId) :
    modelId(modelId), categoryColor(Qt::transparent), hasMixedTypesInExpandedState(false),
    expanded(false), hidden(false), expandedRowCount(1), collapsedRowCount(1)
{
}

TimelineModel::TimelineModel(TimelineModelAggregator *parent) :
    QObject(parent), d(std::make_unique<TimelineModelPrivate>(parent->generateModelId()))
{
    connect(this, &TimelineModel::contentChanged, this, &TimelineModel::labelsChanged);
    connect(this, &TimelineModel::contentChanged, this, &TimelineModel::detailsChanged);
    connect(this, &TimelineModel::hiddenChanged, this, &TimelineModel::heightChanged);
    connect(this, &TimelineModel::expandedChanged, this, &TimelineModel::heightChanged);
    connect(this, &TimelineModel::expandedRowHeightChanged, this, &TimelineModel::heightChanged);
    connect(this, &TimelineModel::expandedChanged, this, &TimelineModel::rowCountChanged);
    connect(this, &TimelineModel::contentChanged, this, &TimelineModel::rowCountChanged);
    connect(this, &TimelineModel::contentChanged,
            this, [this]() { emit expandedRowHeightChanged(-1, -1); });
}

TimelineModel::~TimelineModel()
{
}

bool TimelineModel::isEmpty() const
{
    return count() == 0;
}

int TimelineModel::modelId() const
{
    return d->modelId;
}

int TimelineModel::collapsedRowHeight(int rowNumber) const
{
    Q_UNUSED(rowNumber)
    return TimelineModelPrivate::DefaultRowHeight;
}

int TimelineModel::collapsedRowOffset(int rowNumber) const
{
    return rowNumber * TimelineModelPrivate::DefaultRowHeight;
}

int TimelineModel::expandedRowHeight(int rowNumber) const
{
    if (d->rowOffsets.size() > rowNumber)
        return d->rowOffsets[rowNumber] - (rowNumber > 0 ? d->rowOffsets[rowNumber - 1] : 0);
    return TimelineModelPrivate::DefaultRowHeight;
}

int TimelineModel::expandedRowOffset(int rowNumber) const
{
    if (rowNumber == 0)
        return 0;

    if (d->rowOffsets.size() >= rowNumber)
        return d->rowOffsets[rowNumber - 1];
    if (!d->rowOffsets.empty())
        return d->rowOffsets.last() + (rowNumber - d->rowOffsets.size()) *
                TimelineModelPrivate::DefaultRowHeight;
    return rowNumber * TimelineModelPrivate::DefaultRowHeight;
}

void TimelineModel::setExpandedRowHeight(int rowNumber, int height)
{
    if (height < TimelineModelPrivate::DefaultRowHeight)
        height = TimelineModelPrivate::DefaultRowHeight;

    int nextOffset = d->rowOffsets.empty() ? 0 : d->rowOffsets.last();
    while (d->rowOffsets.size() <= rowNumber)
        d->rowOffsets << (nextOffset += TimelineModelPrivate::DefaultRowHeight);
    int difference = height - d->rowOffsets[rowNumber] +
            (rowNumber > 0 ? d->rowOffsets[rowNumber - 1] : 0);
    if (difference != 0) {
        for (int offsetRow = rowNumber; offsetRow < d->rowOffsets.size(); ++offsetRow) {
            d->rowOffsets[offsetRow] += difference;
        }
        emit expandedRowHeightChanged(rowNumber, height);
    }
}

int TimelineModel::rowOffset(int rowNumber) const
{
    return expanded() ? expandedRowOffset(rowNumber) : collapsedRowOffset(rowNumber);
}

int TimelineModel::rowHeight(int rowNumber) const
{
    return expanded() ? expandedRowHeight(rowNumber) : collapsedRowHeight(rowNumber);
}

int TimelineModel::height() const
{
    if (d->hidden || isEmpty())
        return 0;

    if (!d->expanded)
        return collapsedRowCount() * TimelineModelPrivate::DefaultRowHeight;
    if (d->rowOffsets.empty())
        return expandedRowCount() * TimelineModelPrivate::DefaultRowHeight;

    return d->rowOffsets.last() + (expandedRowCount() - d->rowOffsets.size()) *
            TimelineModelPrivate::DefaultRowHeight;
}

/*!
    Returns the number of ranges in the model.
*/
int TimelineModel::count() const
{
    return d->ranges.count();
}

qint64 TimelineModel::duration(int index) const
{
    return d->ranges[index].duration;
}

qint64 TimelineModel::startTime(int index) const
{
    return d->ranges[index].start;
}

qint64 TimelineModel::endTime(int index) const
{
    return d->ranges[index].start + d->ranges[index].duration;
}

/*!
    Returns the type ID of the event with event ID \a index. The type ID is a globally valid ID
    which can be used to communicate meta information about events to other parts of the program. By
    default it is -1, which means there is no global type information about the event.
 */
int TimelineModel::typeId(int index) const
{
    Q_UNUSED(index)
    return -1;
}

/*!
    Looks up the first range with an end time later than the given time and
    returns its parent's index. If no such range is found, it returns -1. If there
    is no parent, it returns the found range's index. The parent of a range is the
    range with the earliest start time that completely covers the child range.
    "Completely covers" means:
    parent.startTime <= child.startTime && parent.endTime >= child.endTime
*/
int TimelineModel::firstIndex(qint64 startTime) const
{
    int index = d->firstIndexNoParents(startTime);
    if (index == -1)
        return -1;
    int parent = d->ranges[index].parent;
    return parent == -1 ? index : parent;
}

/*!
    Of all indexes of ranges starting at the same time as the first range with an end time later
    than the specified \a startTime returns the lowest one. If no such range is found, it returns
    -1.
*/
int TimelineModel::TimelineModelPrivate::firstIndexNoParents(qint64 startTime) const
{
    // in the "endtime" list, find the first event that ends after startTime

    // lowerBound() cannot deal with empty lists, and it never finds the last element.
    if (endTimes.isEmpty() || endTimes.last().end <= startTime)
        return -1;

    // lowerBound() never returns "invalid", so handle this manually.
    if (endTimes.first().end > startTime)
        return endTimes.first().startIndex;

    return endTimes[lowerBound(endTimes, startTime) + 1].startIndex;
}

/*!
    Looks up the last range with a start time earlier than the specified \a endTime and
    returns its index. If no such range is found, it returns -1.
*/
int TimelineModel::lastIndex(qint64 endTime) const
{
    // in the "starttime" list, find the last event that starts before endtime

    // lowerBound() never returns "invalid", so handle this manually.
    if (d->ranges.isEmpty() || d->ranges.first().start >= endTime)
        return -1;

    // lowerBound() never finds the last element.
    if (d->ranges.last().start < endTime)
        return d->ranges.count() - 1;

    return d->lowerBound(d->ranges, endTime);
}

/*!
    Looks up a range between the last one that starts before, and the first one that ends after the
    given timestamp. This might not be a range that covers the timestamp, even if one exists.
    However, it's likely that the range is close to the given timestamp.
 */
int TimelineModel::bestIndex(qint64 timestamp) const
{
    if (d->ranges.isEmpty())
        return -1;

    // Last range that starts before timestamp (without parents)
    const int start = d->ranges.last().start < timestamp
            ? d->ranges.count() - 1 : d->lowerBound(d->ranges, timestamp);

    int endTimeIndex;
    if (d->endTimes.first().end >= timestamp)
        endTimeIndex = 0;
    else if (d->endTimes.last().end < timestamp)
        endTimeIndex = d->endTimes.count() - 1;
    else
        endTimeIndex = d->lowerBound(d->endTimes, timestamp) + 1;

    // First range that ends after
    const int end = d->endTimes[endTimeIndex].startIndex;

    // Best is probably between those
    return (start + end) / 2;
}

int TimelineModel::parentIndex(int index) const
{
    return d->ranges[index].parent;
}

QVariantMap TimelineModel::location(int index) const
{
    Q_UNUSED(index)
    QVariantMap map;
    return map;
}

/*!
    Returns \c true if this model can contain events of global type ID \a typeIndex. Otherwise
    returns \c false. The base model does not know anything about type IDs and always returns
    \c false. You should override this method if you implement \l typeId().
 */
bool TimelineModel::handlesTypeId(int typeIndex) const
{
    Q_UNUSED(typeIndex)
    return false;
}

float TimelineModel::relativeHeight(int index) const
{
    Q_UNUSED(index)
    return 1.0f;
}

qint64 TimelineModel::rowMinValue(int rowNumber) const
{
    Q_UNUSED(rowNumber)
    return 0;
}

qint64 TimelineModel::rowMaxValue(int rowNumber) const
{
    Q_UNUSED(rowNumber)
    return 0;
}

int TimelineModel::defaultRowHeight()
{
    return TimelineModelPrivate::DefaultRowHeight;
}

QList<const TimelineRenderPass *> TimelineModel::supportedRenderPasses() const
{
    QList<const TimelineRenderPass *> passes;
    passes << TimelineItemsRenderPass::instance()
           << TimelineSelectionRenderPass::instance()
           << TimelineNotesRenderPass::instance();
    return passes;
}

QRgb TimelineModel::colorBySelectionId(int index) const
{
    return colorByHue(selectionId(index) * TimelineModelPrivate::SelectionIdHueMultiplier);
}

QRgb TimelineModel::colorByFraction(double fraction) const
{
    return colorByHue(fraction * TimelineModelPrivate::FractionHueMultiplier +
                      TimelineModelPrivate::FractionHueMininimum);
}

QRgb TimelineModel::colorByHue(int hue) const
{
    return TimelineModelPrivate::hueTable[hue];
}

/*!
    Inserts the range defined by \a duration and \a selectionId at the specified \a startTime and
    returns its index. The \a selectionId determines the selection group the new event belongs to.
    \sa selectionId()
*/
int TimelineModel::insert(qint64 startTime, qint64 duration, int selectionId)
{
    /* Doing insert-sort here is preferable as most of the time the times will actually be
     * presorted in the right way. So usually this will just result in appending. */
    int index = d->insertStart(TimelineModelPrivate::Range(startTime, duration, selectionId));
    if (index < d->ranges.size() - 1)
        d->incrementStartIndices(index);
    int endIndex = d->insertEnd(TimelineModelPrivate::RangeEnd(index, startTime + duration));
    d->setEndIndex(index, endIndex);
    if (endIndex < d->endTimes.size() - 1)
        d->incrementEndIndices(endIndex);
    return index;
}

/*!
    Inserts the specified \a selectionId as range start at the specified \a startTime and returns
    its index. The range end is not set. The \a selectionId determines the selection group the new
    event belongs to.
    \sa selectionId()
*/
int TimelineModel::insertStart(qint64 startTime, int selectionId)
{
    int index = d->insertStart(TimelineModelPrivate::Range(startTime, 0, selectionId));
    if (index < d->ranges.size() - 1)
        d->incrementStartIndices(index);
    return index;
}

/*!
    Adds the range \a duration at the specified start \a index.
*/
void TimelineModel::insertEnd(int index, qint64 duration)
{
    d->ranges[index].duration = duration;
    int endIndex = d->insertEnd(TimelineModelPrivate::RangeEnd(index, d->ranges[index].start + duration));
    d->setEndIndex(index, endIndex);
    if (endIndex < d->endTimes.size() - 1)
        d->incrementEndIndices(endIndex);
}

bool TimelineModel::expanded() const
{
    return d->expanded;
}

void TimelineModel::setExpanded(bool expanded)
{
    if (expanded != d->expanded) {
        d->expanded = expanded;
        emit expandedChanged();
    }
}

bool TimelineModel::hidden() const
{
    return d->hidden;
}

void TimelineModel::setHidden(bool hidden)
{
    if (hidden != d->hidden) {
        d->hidden = hidden;
        emit hiddenChanged();
    }
}

void TimelineModel::setDisplayName(const QString &displayName)
{
    if (d->displayName != displayName) {
        d->displayName = displayName;
        emit displayNameChanged();
    }
}

QString TimelineModel::displayName() const
{
    return d->displayName;
}

int TimelineModel::rowCount() const
{
    return d->expanded ? d->expandedRowCount : d->collapsedRowCount;
}

QString TimelineModel::tooltip() const
{
    return d->tooltip;
}

void TimelineModel::setTooltip(const QString &text)
{
    d->tooltip = text;
    emit tooltipChanged();
}

QColor TimelineModel::categoryColor() const
{
    return d->categoryColor;
}

void TimelineModel::setCategoryColor(const QColor &color)
{
    d->categoryColor = color;
    emit categoryColorChanged();
}

bool TimelineModel::hasMixedTypesInExpandedState() const
{
    return d->hasMixedTypesInExpandedState;
}

void TimelineModel::setHasMixedTypesInExpandedState(bool value)
{
    d->hasMixedTypesInExpandedState = value;
    emit hasMixedTypesInExpandedStateChanged();
}

QRgb TimelineModel::color(int index) const
{
    Q_UNUSED(index)
    return QRgb();
}

QVariantList TimelineModel::labels() const
{
    return QVariantList();
}

QVariantMap TimelineModel::details(int index) const
{
    Q_UNUSED(index)
    return QVariantMap();
}

/**
 * @brief TimelineModel::orderedDetails returns the title and content for the details popup
 * @param index of the selected item
 * @return QVariantMap containing the fields 'title' (QString) and 'content' (QVariantList
 * with alternating keys and values as QStrings)
 */
QVariantMap TimelineModel::orderedDetails(int index) const
{
    QVariantMap info = details(index);
    QVariantMap data;
    QVariantList content;
    auto it = info.constBegin();
    auto end = info.constEnd();
    while (it != end) {
        if (it.key() == "displayName") {
            data.insert("title", it.value());
        } else {
            content.append(it.key());
            content.append(it.value());
        }
        ++it;
    }
    data.insert("content", content);
    return data;
}

int TimelineModel::expandedRow(int index) const
{
    Q_UNUSED(index)
    return 0;
}

int TimelineModel::collapsedRow(int index) const
{
    Q_UNUSED(index)
    return 0;
}

/*!
    Returns the ID of the selection group the event with event ID \a index belongs to. Selection
    groups are local to the model and the model can arbitrarily assign events to selection groups
    when inserting them.
    If one event from a selection group is selected, all visible other events from the same
    selection group are highlighted. Rows are expected to correspond to selection IDs when the view
    is expanded.
 */
int TimelineModel::selectionId(int index) const
{
    return d->ranges[index].selectionId;
}

void TimelineModel::clear()
{
    setExpandedRowCount(1);
    setCollapsedRowCount(1);
    setExpanded(false);
    setHidden(false);
    d->rowOffsets.clear();
    d->ranges.clear();
    d->endTimes.clear();
    emit contentChanged();
}

int TimelineModel::nextItemBySelectionId(int selectionId, qint64 time, int currentItem) const
{
    return d->nextItemById([this, selectionId](int index) {
        return d->ranges[index].selectionId == selectionId;
    }, time, currentItem);
}

int TimelineModel::nextItemByTypeId(int requestedTypeId, qint64 time, int currentItem) const
{
    return d->nextItemById([this, requestedTypeId](int index) {
        return typeId(index) == requestedTypeId;
    }, time, currentItem);
}

int TimelineModel::prevItemBySelectionId(int selectionId, qint64 time, int currentItem) const
{
    return d->prevItemById([this, selectionId](int index) {
        return d->ranges[index].selectionId == selectionId;
    }, time, currentItem);
}

int TimelineModel::prevItemByTypeId(int requestedTypeId, qint64 time, int currentItem) const
{
    return d->prevItemById([this, requestedTypeId](int index) {
        return typeId(index) == requestedTypeId;
    }, time, currentItem);
}

HueLookupTable::HueLookupTable() {
    for (int hue = 0; hue < 360; ++hue) {
        table[hue] = QColor::fromHsl(hue, TimelineModel::TimelineModelPrivate::Saturation,
                                     TimelineModel::TimelineModelPrivate::Lightness).rgb();
    }
}

const HueLookupTable TimelineModel::TimelineModelPrivate::hueTable;

int TimelineModel::TimelineModelPrivate::nextItemById(std::function<bool(int)> matchesId,
                                                      qint64 time, int currentItem) const
{
    if (ranges.empty())
        return -1;

    int ndx = -1;
    if (currentItem == -1)
        ndx = firstIndexNoParents(time);
    else
        ndx = currentItem + 1;

    if (ndx < 0 || ndx >= ranges.count())
        ndx = 0;
    int startIndex = ndx;
    do {
        if (matchesId(ndx))
            return ndx;
        ndx = (ndx + 1) % ranges.count();
    } while (ndx != startIndex);
    return -1;
}

int TimelineModel::TimelineModelPrivate::prevItemById(std::function<bool(int)> matchesId,
                                                      qint64 time, int currentItem) const
{
    if (ranges.empty())
        return -1;

    int ndx = -1;
    if (currentItem == -1)
        ndx = firstIndexNoParents(time);
    else
        ndx = currentItem - 1;
    if (ndx < 0)
        ndx = ranges.count() - 1;
    int startIndex = ndx;
    do {
        if (matchesId(ndx))
            return ndx;
        if (--ndx < 0)
            ndx = ranges.count()-1;
    } while (ndx != startIndex);
    return -1;
}

} // namespace Timeline

#include "moc_timelinemodel.cpp"
