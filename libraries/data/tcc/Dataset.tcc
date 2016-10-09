////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Machine Learning Library (EMLL)
//  File:     Dataset.tcc (data)
//  Authors:  Ofer Dekel
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// utilities
#include "Exception.h"

// stl
#include <algorithm>
#include <cassert>
#include <random>
#include <stdexcept>

namespace emll
{
namespace data
{

    template<typename IteratorExampleType>
    GetIteratorAbstractor<IteratorExampleType>::GetIteratorAbstractor(size_t fromIndex, size_t size) : _fromIndex(fromIndex), _size(size) 
    {}

    template<typename IteratorExampleType>
    template<typename DatasetType>
    auto GetIteratorAbstractor<IteratorExampleType>::operator()(const DatasetType& dataset) const -> ReturnType
    {
        return dataset.GetIterator<IteratorExampleType>(_fromIndex, _size);
    }

    template<typename ExampleType>
    ExampleIterator<ExampleType> AnyDataset::GetIterator()
    {
        GetIteratorAbstractor<ExampleType> abstractor(_fromIndex, _size);
        return utilities::AbstractInvoker<IDataset, Dataset<data::AutoSupervisedExample>, Dataset<data::DenseSupervisedExample>>::Invoke(abstractor, *_pDataset);
    }

    template<typename DatasetExampleType>
    template<typename IteratorExampleType>
    Dataset<DatasetExampleType>::DatasetExampleIterator<IteratorExampleType>::DatasetExampleIterator(InternalIteratorType begin, InternalIteratorType end) : _current(begin), _end(end) 
    {}

    template <typename DatasetExampleType>
    Dataset<DatasetExampleType>::Dataset(ExampleIterator<DatasetExampleType> exampleIterator)
    {
        while (exampleIterator.IsValid())
        {
            AddExample(DatasetExampleType(exampleIterator.Get())); // TODO fix this 
            exampleIterator.Next();
        }
    }

    template <typename DatasetExampleType>
    DatasetExampleType& Dataset<DatasetExampleType>::GetExample(size_t index)
    {
        return _examples[index];
    }

    template <typename DatasetExampleType>
    const DatasetExampleType& Dataset<DatasetExampleType>::GetExample(size_t index) const
    {
        return _examples[index];
    }

    template <typename DatasetExampleType>
    DatasetExampleType& Dataset<DatasetExampleType>::operator[](size_t index)
    {
        return _examples[index];
    }

    template <typename DatasetExampleType>
    const DatasetExampleType& Dataset<DatasetExampleType>::operator[](size_t index) const
    {
        return _examples[index];
    }

    template <typename DatasetExampleType>
    template <typename IteratorExampleType>
    ExampleIterator<IteratorExampleType> Dataset<DatasetExampleType>::GetIterator(size_t fromRowIndex, size_t size) const
    {
        size = CorrectRangeSize(fromRowIndex, size);
        return ExampleIterator<IteratorExampleType>(std::make_shared<DatasetExampleIterator<IteratorExampleType>>(_examples.cbegin() + fromRowIndex, _examples.cbegin() + fromRowIndex + size)); // TODO unique or shared?
    }

    template <typename DatasetExampleType>
    void Dataset<DatasetExampleType>::AddExample(DatasetExampleType example)
    {
        size_t size = example.GetDataVector().Size();
        _examples.push_back(std::move(example));

        if (_maxExampleSize < size)
        {
            _maxExampleSize = size;
        }
    }

    template <typename DatasetExampleType>
    void Dataset<DatasetExampleType>::Reset()
    {
        _examples.clear();
        _maxExampleSize = 0;
    }

    template <typename DatasetExampleType>
    void Dataset<DatasetExampleType>::RandomPermute(std::default_random_engine& rng, size_t prefixSize)
    {
        prefixSize = CorrectRangeSize(0, prefixSize);
        for (size_t i = 0; i < prefixSize; ++i)
        {
            RandomSwap(rng, i, i, _examples.size() - i);
        }
    }

    template <typename DatasetExampleType>
    void Dataset<DatasetExampleType>::RandomPermute(std::default_random_engine& rng, size_t rangeFirstIndex, size_t rangeSize, size_t prefixSize)
    {
        rangeSize = CorrectRangeSize(rangeFirstIndex, rangeSize);

        if (prefixSize > rangeSize || prefixSize == 0)
        {
            prefixSize = rangeSize;
        }

        for (size_t s = 0; s < prefixSize; ++s)
        {
            size_t index = rangeFirstIndex + s;
            RandomSwap(rng, index, index, rangeSize - s);
        }
    }

    template <typename DatasetExampleType>
    void Dataset<DatasetExampleType>::RandomSwap(std::default_random_engine& rng, size_t targetExampleIndex, size_t rangeFirstIndex, size_t rangeSize)
    {
        using std::swap;
        rangeSize = CorrectRangeSize(rangeFirstIndex, rangeSize);
        if (targetExampleIndex > _examples.size())
        {
            throw utilities::InputException(utilities::InputExceptionErrors::indexOutOfRange);
        }

        std::uniform_int_distribution<size_t> dist(rangeFirstIndex, rangeFirstIndex + rangeSize - 1);
        size_t j = dist(rng);
        swap(_examples[targetExampleIndex], _examples[j]);
    }

    template <typename DatasetExampleType>
    template <typename SortKeyType>
    void Dataset<DatasetExampleType>::Sort(SortKeyType sortKey, size_t fromRowIndex, size_t size)
    {
        size = CorrectRangeSize(fromRowIndex, size);

        std::sort(_examples.begin() + fromRowIndex,
                  _examples.begin() + fromRowIndex + size,
                  [&](const DatasetExampleType& a, const DatasetExampleType& b) -> bool {
                      return sortKey(a) < sortKey(b);
                  });
    }

    template <typename DatasetExampleType>
    template <typename PartitionKeyType>
    void Dataset<DatasetExampleType>::Partition(PartitionKeyType partitionKey, size_t fromRowIndex, size_t size)
    {
        size = CorrectRangeSize(fromRowIndex, size);
        std::partition(_examples.begin() + fromRowIndex, _examples.begin() + fromRowIndex + size, partitionKey);
    }

    template <typename DatasetExampleType>
    void Dataset<DatasetExampleType>::Print(std::ostream& os, size_t tabs, size_t fromRowIndex, size_t size) const
    {
        size = CorrectRangeSize(fromRowIndex, size);

        for (size_t index = fromRowIndex; index < fromRowIndex + size; ++index)
        {
            os << std::string(tabs * 4, ' ');
            _examples[index].Print(os);
            os << "\n";
        }
    }

    template<typename ExampleType>
    Dataset<ExampleType> MakeDataset(ExampleIterator<ExampleType> iterator)
    {
        return Dataset<ExampleType>(iterator);
    }

    template <typename DatasetExampleType>
    std::ostream& operator<<(std::ostream& os, const Dataset<DatasetExampleType>& dataset)
    {
        dataset.Print(os);
        return os;
    }

    template <typename DatasetExampleType>
    size_t Dataset<DatasetExampleType>::CorrectRangeSize(size_t fromRowIndex, size_t size) const
    {
        if (size == 0 || fromRowIndex + size > _examples.size())
        {
            return _examples.size() - fromRowIndex;
        }
        return size;
    }
}
}
