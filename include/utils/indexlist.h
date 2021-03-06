//=================================================================================================
/*!
//  This file is part of the Pannon Optimizer library. 
//  This library is free software; you can redistribute it and/or modify it under the 
//  terms of the GNU Lesser General Public License as published by the Free Software 
//  Foundation; either version 3.0, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
//  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//  See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License; see the file 
//  COPYING. If not, see http://www.gnu.org/licenses/.
*/
//=================================================================================================

/**
 * @file linkedlist.h This file contains the API of the IndexList class.
 * @author Jozsef Smidla
 */


#ifndef LINKEDLIST_H
#define	LINKEDLIST_H

#include <globals.h>
#include <utils/numerical.h>

#include <set>
#include <iostream>
#include <typeinfo>
#include <vector>

//#define INDEXLIST_LOGGING

using namespace std;

struct unused { int operator*() const {return 0;}};

/**
 * The class stores double linked lists of indices.
 * Each linked list has a header. The next and previous element
 * of an empty linked list is the header. When the linked is not empty,
 * the next element of the last element is the header, and the previous
 * element of first element is the header also.
 *
 * @class IndexList
 */
template <class ATTACHED_TYPE = unused>
class IndexList
{
    friend class IndexListTestSuite;

    /**
     * Stores an element of the linked list.
     */
    template <class TYPE>
    struct Element
    {
        /**
         * This variable represents the stored index.
         */
        unsigned int m_data;

        /**
         * Link to the next element in the linked list.
         */
        Element * m_next;

        /**
         * Link to the previous element in the linked list.
         */
        Element * m_previous;

        /**
         * When the element represnts a header, it is true.
         */
        bool m_isHeader;

        /**
         * The current element belongs to the m_partitionIndex'th partition.
         * When an index does not belongs to a partition, m_partitionIndex is
         * greater or equal than the number of partitions.
         */
        unsigned int m_partitionIndex;

        /**
         * An attached object.
         */
        TYPE m_attached;
    };

public:

    /**
     * Constructor of IndexList.
     * Initializes the linked lists to empty. The count and partitions can be zero.
     * <hr>
     * </pre>
     *
     * <table>
     * <tr align="center">
     *   <th>count</th>
     *   <th>partitions</th>
     *   <th>complexity</th>
     *   <th>validity</th>
     * </tr>
     * <tr align="center">
     *   <td> zero </td>
     *   <td> zero </td>
     *   <td> O(1) </td>
     *   <td> valid </td>
     * </tr>
     * <tr align="center">
     *   <td> nonzero </td>
     *   <td> zero </td>
     *   <td> - </td>
     *   <td> invalid </td>
     * </tr>
     * <tr align="center">
     *   <td> zero </td>
     *   <td> nonzero </td>
     *   <td> - </td>
     *   <td> invalid </td>
     * </tr>
     * <tr align="center">
     *   <td> nonzero </td>
     *   <td> nonzero </td>
     *   <td> O(count + partitions) </td>
     *   <td> valid </td>
     * </tr>
     * </table>
     *
     * @constructor
     * @param count Number of possible indices.
     * @param partitions Number of linked lists.
     */
    IndexList(unsigned int count = 0, unsigned int partitions = 0) {
        m_partitionCount = 0;
        m_heads = 0;
        m_count = 0;
        m_dataArray = 0;
        if (partitions != 0 && count != 0) {
            init(count, partitions);
        }
    }

    /**
     * Copy constructor of IndexList.
     * <hr>
     * </pre>
     *
     * <table>
     * <tr align="center">
     *   <th>list.count</th>
     *   <th>list.partitions</th>
     *   <th>complexity</th>
     *   <th>validity</th>
     * </tr>
     * <tr align="center">
     *   <td> zero </td>
     *   <td> zero </td>
     *   <td> O(1) </td>
     *   <td> valid </td>
     * </tr>
     * <tr align="center">
     *   <td> nonzero </td>
     *   <td> zero </td>
     *   <td> - </td>
     *   <td> invalid </td>
     * </tr>
     * <tr align="center">
     *   <td> zero </td>
     *   <td> nonzero </td>
     *   <td> - </td>
     *   <td> invalid </td>
     * </tr>
     * <tr align="center">
     *   <td> nonzero </td>
     *   <td> nonzero </td>
     *   <td> O(list.count + list.partitions) </td>
     *   <td> valid </td>
     * </tr>
     * </table>
     *
     * @constructor
     * @param list The original list.
     */
    IndexList(const IndexList & list) = delete;
//    {
//        copy(list);
//    }

    /**
     * Assignment operator of IndexList.
     * <hr>
     * </pre>
     *
     * <table>
     * <tr align="center">
     *   <th>list.count</th>
     *   <th>list.partitions</th>
     *   <th>complexity</th>
     *   <th>validity</th>
     * </tr>
     * <tr align="center">
     *   <td> zero </td>
     *   <td> zero </td>
     *   <td> O(1) </td>
     *   <td> valid </td>
     * </tr>
     * <tr align="center">
     *   <td> nonzero </td>
     *   <td> zero </td>
     *   <td> - </td>
     *   <td> invalid </td>
     * </tr>
     * <tr align="center">
     *   <td> zero </td>
     *   <td> nonzero </td>
     *   <td> - </td>
     *   <td> invalid </td>
     * </tr>
     * <tr align="center">
     *   <td> nonzero </td>
     *   <td> nonzero </td>
     *   <td> O(list.count + list.partitions) </td>
     *   <td> valid </td>
     * </tr>
     * </table>
     *
     * @param list The original list object.
     * @return Reference of the actual list object.
     */
    IndexList & operator=(const IndexList & list) = delete;
//    {
//        clear();
//        copy(list);
//        return *this;
//    }

    /**
     * Destructor of IndexList.
     * <hr>
     * Complexity: O(1)
     *
     * @destructor
     */
    ~IndexList() {
        clear();
    }

    /**
     * Returns with the number of linked lists.
     * <hr>
     * Complexity: O(1)
     *
     * @return The number of linked lists
     */
    inline unsigned int getPartitionCount() const
    {
        return m_partitionCount;
    }

    /**
     * Returns with the number of possible indices.
     * <hr>
     * Complexity: O(1)
     *
     * @return The number of possible indices
     */
    inline unsigned int getIndexCount() const
    {
        return m_count;
    }

    /**
     * Initializes the linked lists and the set of possible indices.
     * The possible indices are nonnegative values, and less than count.
     * <hr>
     * </pre>
     *
     * <table>
     * <tr align="center">
     *   <th>count</th>
     *   <th>partitions</th>
     *   <th>complexity</th>
     *   <th>validity</th>
     * </tr>
     * <tr align="center">
     *   <td> zero </td>
     *   <td> zero </td>
     *   <td> O(1) </td>
     *   <td> valid </td>
     * </tr>
     * <tr align="center">
     *   <td> nonzero </td>
     *   <td> zero </td>
     *   <td> - </td>
     *   <td> invalid </td>
     * </tr>
     * <tr align="center">
     *   <td> zero </td>
     *   <td> nonzero </td>
     *   <td> - </td>
     *   <td> invalid </td>
     * </tr>
     * <tr align="center">
     *   <td> nonzero </td>
     *   <td> nonzero </td>
     *   <td> O(count + partitions) </td>
     *   <td> valid </td>
     * </tr>
     * </table>
     *
     * @param count Number of possible indices.
     * @param partitions Number of linked lists.
     */
    void init(unsigned int count, unsigned int partitions) {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("init( " + std::to_string(count) + ", " +
                        std::to_string(partitions) + " )");
#endif
        clear();
        Element<ATTACHED_TYPE> * pointerIterator;
        Element<ATTACHED_TYPE> * pointerIteratorEnd;
        m_partitionCount = partitions;
        m_heads = new Element<ATTACHED_TYPE>[partitions];
        pointerIterator = m_heads;
        pointerIteratorEnd = m_heads + partitions;
        unsigned int index = 0;
        for (; pointerIterator < pointerIteratorEnd; pointerIterator++, index++) {
            pointerIterator->m_data = index;
            pointerIterator->m_next = pointerIterator;
            pointerIterator->m_previous = pointerIterator;
            pointerIterator->m_isHeader = true;
            pointerIterator->m_partitionIndex = index;
            pointerIterator->m_attached = ATTACHED_TYPE();
        }

        m_count = count;
        m_dataArray = new Element<ATTACHED_TYPE>[count];
        pointerIterator = m_dataArray;
        pointerIteratorEnd = m_dataArray + count;
        index = 0;
        for (; pointerIterator < pointerIteratorEnd; pointerIterator++, index++) {
            pointerIterator->m_data = index;
            pointerIterator->m_next = 0;
            pointerIterator->m_previous = 0;
            pointerIterator->m_isHeader = false;
            pointerIterator->m_partitionIndex = partitions;
            pointerIterator->m_attached = ATTACHED_TYPE();
        }
    }

    /**
     * Inserts an index to the linked list given by argument partitionIndex.
     * Value must not be any of linked list.
     * <hr>
     * Complexity: O(1)
     *
     * @param partitionIndex The index will be inserted to this linked list.
     * @param index This index will be inserted.
     * @param attached The attached data to the index.
     */
    inline void insert(unsigned int partitionIndex, unsigned int index, ATTACHED_TYPE attached = ATTACHED_TYPE())
    {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("insert( " + std::to_string(partitionIndex) + ", " +
                        std::to_string(index) + " )");
#endif
        //LPERROR(m_partitionCount << " / " << partitionIndex );
        /*if (where(index) == partitionIndex) {
            return;
        }*/
        Element<ATTACHED_TYPE> * forward = m_heads[partitionIndex].m_next;
        m_heads[partitionIndex].m_next = m_dataArray + index;
        m_dataArray[index].m_next = forward;
        m_dataArray[index].m_partitionIndex = partitionIndex;
        m_dataArray[index].m_attached = attached;
        forward->m_previous = m_dataArray + index;
        m_dataArray[index].m_previous = m_heads + partitionIndex;
    }

    /**
     * Sets the attached data of the value'th element.
     *
     * @param index The index of the element which data is to be changed.
     * @param data The new attached data of the element.
     */
    inline void setAttachedData(unsigned int index, ATTACHED_TYPE data) {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("setAttachedData( " + std::to_string(index) + " )");
#endif
        m_dataArray[index].m_attached = data;
    }

    /**
     * Returns the attached data of a specific element.
     *
     * @param index The index of the element.
     * @return The attached data of the element.
     */
    inline ATTACHED_TYPE getAttachedData(unsigned int index) const {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("getAttachedData( " + std::to_string(index) + " )");
#endif
        return m_dataArray[index].m_attached;
    }
    /**
     * Removes the index from the linked lists.
     * <hr>
     * Complexity: O(1)
     *
     * @param index This index will be removed.
     */
    inline void remove(unsigned int index)
    {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("remove( " + std::to_string(index) + " )");
#endif
        Element<ATTACHED_TYPE> & element = m_dataArray[index];
        if (element.m_partitionIndex == m_partitionCount) {
            return;
        }
        element.m_partitionIndex = m_partitionCount;
        element.m_previous->m_next = element.m_next;
        element.m_next->m_previous = element.m_previous;
    }

    /**
     * Moves a specific index to a target partition.
     *
     * @param index The index to be moved.
     * @param targetPartition The index of the partition the index is moved to.
     * @param attached The attached data to the index.
     */
    inline void move(unsigned int index, unsigned int targetPartition, ATTACHED_TYPE attached = ATTACHED_TYPE()) {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("move( " + std::to_string(index) + ", " +
                        std::to_string(targetPartition) + " )");
#endif
        /*Element<ATTACHED_TYPE> & element = m_dataArray[value];
        element.m_partitionIndex = targetPartition;

        Element<ATTACHED_TYPE> * forward = m_heads[targetPartition].m_next;
        m_heads[targetPartition].m_next = &element;
        element.m_next = forward;
        forward->m_previous = &element;
        element.m_previous = m_heads + targetPartition;
        m_dataArray[value].m_attached = attached;*/
        //if (where(index) )
        remove(index);
        insert(targetPartition, index, attached);
    }

    /**
     * Returns the number of partition where a specific index can be found.
     *
     * @param index The function returns with the partition index of this value.
     * @return The partition index.
     *
     * @see m_partitionIndex
     */
    inline unsigned int where(unsigned int index) const {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("where( " + std::to_string(index) + " )");
#endif
        return m_dataArray[index].m_partitionIndex;
    }

    /**
     * Returns the first element of a partition.
     *
     * @param partitionIndex The index of the linked list
     * @return The data of the first element or -1 if the list is empty
     */
    inline int firstElement(unsigned int partitionIndex) const {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("firstElement( " + std::to_string(partitionIndex) + " )");
#endif
        if (&(m_heads[partitionIndex]) == m_heads[partitionIndex].m_next){
            return -1;
        } else {
            return m_heads[partitionIndex].m_next->m_data;
        }
    }

    /**
     * Reverses the order of elements in the given partition.
     *
     * @param partitionIndex The index of the partition
     */
    void reversePartition(unsigned int partitionIndex) {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("reversePartition( " + std::to_string(partitionIndex) + " )");
#endif
        PartitionIterator iter, iterEnd;
        getIterators(&iter, &iterEnd, partitionIndex);
        std::vector<unsigned int> elements;
        std::vector<ATTACHED_TYPE> attached;
        for (; iter != iterEnd; ++iter) {
            elements.push_back(iter.getData());
            attached.push_back(iter.getAttached());
        }
        clearPartition(partitionIndex);
        unsigned int index;
        for (index = 0; index < elements.size(); index++) {
            insert(partitionIndex, elements[index], attached[index]);
        }
    }

    /**
     * Iterator class for listing elements of a linked list.
     *
     * @class _Iterator
     */
    template <class TYPE>
    class _Iterator
    {
        /**
         * Pointer to an element of a linked list.
         */
        Element<TYPE> * m_actual;

        /**
         * The partition borders for the iterator.
         */
        std::set<Element<TYPE>*> m_borders;
    public:

        /**
         * Default constructor of class Iterator.
         * <hr>
         * Complexity: O(1)
         *
         * @constructor
         */
        _Iterator()
        {
            m_actual = 0;
        }

        /**
         * Constructor of class Iterator. Sets the pointer to an element of a linked list.
         *
         * @constructor
         * @param actual The value of the pointer of actual element in linked list.
         */
        _Iterator(Element<TYPE> * actual)
        {
            m_actual = actual;
        }

        /**
         * Constructor of class Iterator. Sets the pointer to an element
         * of a linked list.
         * <hr>
         * Complexity: O(1)
         *
         * @constructor
         * @param actual The value of pointer of actual element in linked list.
         * @param borders The set of headers of partitions in case of multiple-partition iteration.
         */
        _Iterator(Element<TYPE> * actual, const std::set<Element<TYPE>*> & borders)
        {
            m_actual = actual;
            m_borders = borders;
        }

        /**
         * Returns the index of the actual element.
         * When the iterator refers to the header of linked list,
         * this functions returns the index of linked list.
         * <hr>
         * Complecity: O(1)
         *
         * @return Index of linked list or actual element.
         */
        ALWAYS_INLINE unsigned int getData() const
        {
            return m_actual->m_data;
        }

        /**
         * Returns with the stored attached data of the current element.
         *
         * @return The stored attached data.
         */
        ALWAYS_INLINE const TYPE & getAttached() const{
            return m_actual->m_attached;
        }

        /**
         * Sets the attached value of the actual element.
         *
         * @param attached The attached value
         */
        ALWAYS_INLINE void setAttached(const TYPE & attached) {
            m_actual->m_attached = attached;
        }

        /**
         * Returns with the partition index of the current element.
         *
         * @return Partition index of the current element.
         */
        ALWAYS_INLINE unsigned int getPartitionIndex() const {
            return m_actual->m_partitionIndex;
        }

        /**
         * Moves the iterator to the next element.
         * When the iterator refers to the last element, the iterator steps to the header.
         * <hr>
         * Complexity: O(1)
         */
        void next()
        {
            m_actual = m_actual->m_next;
            if (m_actual->m_isHeader == true) {
                typename std::set<Element<TYPE>*>::iterator iter = m_borders.find(m_actual);
                if (iter != m_borders.end()) {
                    ++iter; // next border
                    if (iter != m_borders.end()) {
                        m_actual = (*iter)->m_next;
                        bool end = !m_actual->m_isHeader;
                        while (end == false) {
                            iter = m_borders.find(m_actual);
                            if (iter != m_borders.end()) {
                                ++iter;
                                if (iter != m_borders.end()) {
                                    m_actual = (*iter)->m_next;
                                    end = !m_actual->m_isHeader;
                                } else {
                                    end = true;
                                }
                            } else {
                                end = true;
                            }
                        }
                    }
                }
            }
        }

        /**
         * Moves the iterator to the previous element.
         * When the iterator refers to the first element, the iterator steps to the header.
         * <hr>
         * Complexity: O(1)
         */
        void previous()
        {
            m_actual = m_actual->m_previous;
        }

        /**
         * Moves the iterator to the next element.
         * When the iterator refers to the last element, the iterator steps to the header.
         * <hr>
         * Complexity: O(1)
         *
         * @return Reference to the iterator object.
         */
        ALWAYS_INLINE _Iterator & operator++()
        {
            next();
            return *this;
        }

        /**
         * Moves the iterator to the previous element.
         * When the iterator refers to the first element, the iterator steps to the header.
         * <hr>
         * Complexity: O(1)
         *
         * @return Reference to the iterator object.
         */
        ALWAYS_INLINE _Iterator & operator--()
        {
            previous();
            return *this;
        }

        /**
         * Returns true when the iter and current Iterator refer to the same list element.
         * <hr>
         * Complexity: O(1)
         *
         * @param iter The other Iterator object.
         * @return True, when the 2 iterators refer to the same list element.
         */
        ALWAYS_INLINE bool operator==(const _Iterator & iter)
        {
            return m_actual == iter.m_actual;
        }

        /**
         * Returns true when the iter and current Iterator refer to different list elements.
         * <hr>
         * Complexity: O(1)
         *
         * @param iter The other Iterator object.
         * @return True, when the 2 iterators refer to different list element.
         */
        ALWAYS_INLINE bool operator!=(const _Iterator & iter)
        {
            return m_actual != iter.m_actual;
        }

    };

    /**
     * Iterator class for listing elements of a single partition of a linked list.
     *
     * @class _PartitionIterator
     */
    template <class TYPE>
    class _PartitionIterator
    {
        /**
         * Pointer to an element of a linked list.
         */
        Element<TYPE> * m_actual;

    public:

        /**
         * Default constructor of class PartitionIterator.
         * <hr>
         * Complexity: O(1)
         *
         * @constructor
         */
        _PartitionIterator()
        {
            m_actual = 0;
        }

        /**
         * Constructor of class PartitionIterator. Sets the pointer to an element
         * of a linked list.
         * <hr>
         * Complexity: O(1)
         *
         * @constructor
         * @param actual The value of pointer of actual element in linked list.
         */
        _PartitionIterator(Element<TYPE> * actual)
        {
            m_actual = actual;
        }

        /**
         * Returns the index of the actual element.
         * When the iterator refers to the header of linked list,
         * this functions returns the index of linked list.
         * <hr>
         * Complecity: O(1)
         *
         * @return Index of linked list or actual element.
         */
        ALWAYS_INLINE unsigned int getData() const
        {
            return m_actual->m_data;
        }

        /**
         * Returns with the stored attached data of the current element.
         *
         * @return The stored attached data.
         */
        ALWAYS_INLINE const TYPE & getAttached() const{
            return m_actual->m_attached;
        }

        /**
         * Sets the attached value of the actual element.
         *
         * @param attached The attached value
         */
        ALWAYS_INLINE void setAttached(const TYPE & attached) {
            m_actual->m_attached = attached;
        }

        /**
         * Returns with the partition index of the current element.
         *
         * @return Partition index of the current element.
         */
        ALWAYS_INLINE unsigned int getPartitionIndex() const {
            return m_actual->m_partitionIndex;
        }

        /**
         * Moves the iterator to the next element.
         * When the iterator refers to the last element, the iterator steps to the header.
         * <hr>
         * Complexity: O(1)
         */
        ALWAYS_INLINE void next()
        {
            m_actual = m_actual->m_next;
        }

        /**
         * Moves the iterator to the previous element.
         * When the iterator refers to the first element, the iterator steps to the header.
         * <hr>
         * Complexity: O(1)
         */
        ALWAYS_INLINE void previous()
        {
            m_actual = m_actual->m_previous;
        }

        /**
         * Moves the iterator to the next element.
         * When the iterator refers to the last element, the iterator steps to the header.
         * <hr>
         * Complexity: O(1)
         *
         * @return Reference to the iterator object.
         */
        ALWAYS_INLINE _PartitionIterator & operator++()
        {
            next();
            return *this;
        }

        /**
         * Moves the iterator to the previous element.
         * When the iterator refers to the first element, the iterator steps to the header.
         * <hr>
         * Complexity: O(1)
         *
         * @return Reference to the iterator object.
         */
        ALWAYS_INLINE _PartitionIterator & operator--()
        {
            previous();
            return *this;
        }

        /**
         * Returns true when the iter and current Iterator refer to the same list element.
         * <hr>
         * Complexity: O(1)
         *
         * @param iter The other Iterator object.
         * @return True, when the 2 iterators refer to the same list element.
         */
        ALWAYS_INLINE bool operator==(const _PartitionIterator & iter)
        {
            return m_actual == iter.m_actual;
        }

        /**
         * Returns true when the iter and current Iterator refer to different list elements.
         * <hr>
         * Complexity: O(1)
         *
         * @param iter The other Iterator object.
         * @return True, when the 2 iterators refer to different list element.
         */
        ALWAYS_INLINE bool operator!=(const _PartitionIterator & iter)
        {
            return m_actual != iter.m_actual;
        }

        operator _Iterator<ATTACHED_TYPE>() const {
            return _Iterator<ATTACHED_TYPE>(m_actual);
        }

        void dump(std::ostream & os) const {
            os << "m_acutal: " << m_actual << std::endl;

        }
    };

    typedef _Iterator<ATTACHED_TYPE> Iterator;
    typedef _PartitionIterator<ATTACHED_TYPE> PartitionIterator;

    Iterator getIterator() const {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("getIterator()");
#endif
        return Iterator();
    }

    PartitionIterator getPartitionIterator() const {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("getPartitionIterator()");
#endif
        return PartitionIterator();
    }

    /**
     * Gets the start and end iterators for the index list.
     * The starting partition and partition count to be iterated can be also specified.
     *
     * @param begin Pointer to the iterator that will point to the start of the list.
     * @param end Pointer to the iterator that will point to the end of the list.
     * @param partitionIndex The starting partition to be iterated.
     * @param partitions The partition count to be iterated.
     */
    void getIterators(Iterator * begin, Iterator * end, unsigned int partitionIndex,
                      unsigned int partitions = 1) const
    {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("getIterators( " + std::to_string(partitionIndex) +
                        + ", partitions" + std::to_string(partitions) + " )");
#endif
        unsigned int lastPartitionIndex = partitionIndex + partitions - 1;
        Element<ATTACHED_TYPE> * beginHead = m_heads + partitionIndex;
        Element<ATTACHED_TYPE> * endHead = m_heads + lastPartitionIndex;
        std::set<Element<ATTACHED_TYPE>*> borders;
        *end = Iterator(endHead, borders);

        unsigned int index = partitionIndex;
        for (; index <= lastPartitionIndex; index++) {
            borders.insert(m_heads + index);
        }

        *begin = Iterator(beginHead, borders);
        begin->next();
    }

    /**
     * Gets the start and end iterators for a partition of the index list.
     * The iterated partition needs to be specified.
     *
     * @param begin Pointer to the iterator that will point to the start of the partition.
     * @param end Pointer to the iterator that will point to the end of the partition.
     * @param partitionIndex The partition to be iterated.
     */
    void getIterators(PartitionIterator * begin, PartitionIterator * end, unsigned int partitionIndex) const
    {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("getIterators( " + std::to_string(partitionIndex) + " )");
#endif
        *begin = m_heads + partitionIndex;
        //*end = m_heads + partitionIndex;
        *end = *begin;
        begin->next();
    }

    /**
     * Returns true if the specified partition contains no indices.
     *
     * @param index The index of the partition to be queried.
     * @return True if the specified partition contains no indices.
     */
    inline bool isPartitionEmpty(unsigned int index) {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("isPartitionEmpty( " + std::to_string(index) + " )");
#endif
        Element<ATTACHED_TYPE> * element = m_heads + index;
        return element->m_next == element;
    }

    /**
     * Checks whether index is a member of any partition in the list.
     * @param index element to search for
     * @return sucess of the search
     */
    inline bool contains (unsigned int index) {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("contains( " + std::to_string(index) + " )");
#endif
        Iterator it;
        Iterator endit;
        getIterators(&it, &endit ,0 , m_partitionCount);
        for (; it != endit; ++it) {
            if (index == it.getData()) {
                return true;
            }
        }
        return false;
    }

    /**
     * Removes all indices from a specified partition.
     *
     * @param index The index of the partition to be cleared.
     */
    inline void clearPartition(unsigned int index)
    {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("clearPartition( " + std::to_string(index) + " )");
#endif

        Iterator iter, endIter;
        getIterators(&iter, &endIter, index, 1);
        for (; iter != endIter; ++iter) {
            remove( iter.getData() );
        }

        Element<ATTACHED_TYPE> * element = m_heads + index;
        element->m_next = element;
        element->m_previous = element;

    }

    /**
     * Removes all indices from the list.
     */
    void clearAllPartitions() {
#ifdef INDEXLIST_LOGGING
        m_log.push_back("clearAllPartitions");
#endif
        unsigned int index;
        Element<ATTACHED_TYPE> * element = m_heads;
        for (index = 0; index < m_partitionCount; index++, element++) {
            if (element->m_next != element) {
                clearPartition(index);
            }
        }
    }

#ifdef INDEXLIST_LOGGING
    void enableLogging(bool enable) {
        m_enableLog = enable;
    }

    const std::vector<std::string> & getLog() const {
        return m_log;
    }
#endif

private:

    /**
     * Number of linked lists.
     */
    unsigned int m_partitionCount;

    /**
     * Pointer to the array of header pointers.
     * The size of array is m_partitionCount.
     */
    Element<ATTACHED_TYPE> * m_heads;

    /**
     * Number of possible indices.
     */
    unsigned int m_count;

    /**
     * Pointer to the array of indices.
     * The size of array is m_count.
     */
    Element<ATTACHED_TYPE> * m_dataArray;

#ifdef INDEXLIST_LOGGING
    /**
     * Enables or disables logging.
     */
    bool m_enableLog;

    /**
     * Contains the log.
     */
    mutable std::vector<std::string> m_log;
#endif

    /**
     * Releases the arrays, and sets to zero each variable.
     * <hr>
     * Complexity: O(1)
     */
    void clear() {

        delete [] m_heads;
        delete [] m_dataArray;
        m_partitionCount = 0;
        m_count = 0;
        m_heads = 0;
        m_dataArray = 0;
    }

    /**
     * Copies the list.
     * <hr>
     * </pre>
     *
     * <table>
     * <tr align="center">
     *   <th>list.m_count</th>
     *   <th>complexity</th>
     * </tr>
     * <tr align="center">
     *   <td> zero </td>
     *   <td> O(1) </td>
     * </tr>
     * <tr align="center">
     *   <td> nonzero </td>
     *   <td> O(list.m_count + list.m_partitions) </td>
     * </tr>
     * </table>
     *
     * @param list
     */
    //TODO: Joco nézd át pls
    void copy(const IndexList & list) {
        if (list.m_count == 0) {
            m_partitionCount = 0;
            m_heads = 0;
            m_count = 0;
            m_dataArray = 0;
            return;
        }

        Element<ATTACHED_TYPE> * myPointerIterator;
        Element<ATTACHED_TYPE> * myPointerIteratorEnd;
        Element<ATTACHED_TYPE> * hisPointerIterator;
        m_partitionCount = list.m_partitionCount;
        m_heads = new Element<ATTACHED_TYPE>[m_partitionCount];
        m_count = list.m_count;
        m_dataArray = new Element<ATTACHED_TYPE>[m_count];

        myPointerIterator = m_heads;
        myPointerIteratorEnd = m_heads + m_partitionCount;
        hisPointerIterator = list.m_heads;
        unsigned int index = 0;
        for (; myPointerIterator < myPointerIteratorEnd; myPointerIterator++,
             hisPointerIterator++, index++) {
            myPointerIterator->m_data = index;
            if (hisPointerIterator->m_next != hisPointerIterator) {
                myPointerIterator->m_next = m_dataArray + (hisPointerIterator->m_next - list.m_dataArray);
                myPointerIterator->m_previous = m_dataArray + (hisPointerIterator->m_previous - list.m_dataArray);
            } else {
                myPointerIterator->m_next = myPointerIterator;
                myPointerIterator->m_previous = myPointerIterator;
            }

        }

        myPointerIterator = m_dataArray;
        myPointerIteratorEnd = m_dataArray + m_count;
        hisPointerIterator = list.m_dataArray;
        index = 0;
        for (; myPointerIterator < myPointerIteratorEnd; myPointerIterator++,
             hisPointerIterator++, index++) {
            myPointerIterator->m_data = index;
            if (hisPointerIterator->m_next == 0) {
                myPointerIterator->m_next = 0;
                myPointerIterator->m_previous = 0;
            } else {
                if (hisPointerIterator->m_next >= list.m_dataArray &&
                        hisPointerIterator->m_next < list.m_dataArray + m_count) {
                    myPointerIterator->m_next = m_dataArray + (hisPointerIterator->m_next - list.m_dataArray);
                } else {
                    myPointerIterator->m_next = m_heads + (hisPointerIterator->m_next - list.m_heads);
                }

                if (hisPointerIterator->m_previous >= list.m_dataArray &&
                        hisPointerIterator->m_previous < list.m_dataArray + m_count) {
                    myPointerIterator->m_previous = m_dataArray + (hisPointerIterator->m_previous - list.m_dataArray);
                } else {
                    myPointerIterator->m_previous = m_heads + (hisPointerIterator->m_previous - list.m_heads);
                }

            }
        }
    }

};


template <class ATTACHED_TYPE>
std::ostream & operator << (std::ostream & os, const IndexList<ATTACHED_TYPE> & list) {

    os << "Partitions: " << list.getPartitionCount() << std::endl;
    os << "Indices: " << list.getIndexCount() << std::endl;

    unsigned int index;
    for(index = 0; index < list.getPartitionCount(); index++){
        os << "List " << index << ": " << std::endl;

        typename IndexList<ATTACHED_TYPE>::Iterator iter, iterEnd;
        list.getIterators(&iter, &iterEnd, index);
        for (; iter != iterEnd; ++iter) {
            os << "     " << iter.getData();
//            if(typeid(ATTACHED_TYPE) == typeid(double*)){
                os << "  ; val = " << *(iter.getAttached());
//            }
            os << std::endl;
        }
    }
    return os;
}


#endif	/* LINKEDLIST_H */
