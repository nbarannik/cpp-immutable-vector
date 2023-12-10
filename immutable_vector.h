#pragma once

#include <vector>
#include <memory>
#include <optional>

const size_t kBitsInBatch = 5;

template <typename T>
struct Node {
    Node() : children((1 << kBitsInBatch)) {
    }
    Node(const T& value) : children((1 << kBitsInBatch)), value(value) {
    }
    Node(const std::vector<std::shared_ptr<Node<T>>>& children, std::optional<T> val)
        : children(children), value(val) {
    }

    std::vector<std::shared_ptr<Node<T>>> children;
    std::optional<T> value;
};

template <class T>
class ImmutableVector {
public:
    ImmutableVector() : root_(new Node<T>()), size_(0) {
    }

    explicit ImmutableVector(size_t count, const T& value = T())
        : root_(std::make_shared<Node<T>>()), size_(count) {
        ImmutableVector<T> result;
        for (size_t i = 0; i < count; ++i) {
            result = result.PushBack(value);
        }
        Copy(result);
    }

    template <typename Iterator>
    ImmutableVector(Iterator first, Iterator last) {
        ImmutableVector<T> result;
        for (auto it = first; it != last; ++it) {
            result = result.PushBack(*it);
        }
        Copy(result);
    }

    ImmutableVector(std::initializer_list<T> l) {
        ImmutableVector<T> result;
        for (const auto& el : l) {
            result = result.PushBack(el);
        }
        Copy(result);
    }

    ImmutableVector Set(size_t index, const T& value) {
        return ImmutableVector<T>(SetValue(root_, index, 0, GetMaxBatchId(index), value), size_);
    }

    const T& Get(size_t index) const {
        return GetValue(root_, index, 0, GetMaxBatchId(index));
    }

    ImmutableVector PushBack(const T& value) {
        return ImmutableVector<T>(Insert(root_, size_, 0, GetMaxBatchId(size_), value), size_ + 1);
    }

    ImmutableVector PopBack() {
        if (size_ == 0) {
            throw std::out_of_range("Cannot pop_back from empty vector");
        }
        return ImmutableVector<T>(Erase(root_, size_, 0, GetMaxBatchId(size_)), size_ - 1);
    }

    size_t Size() const {
        return size_;
    }

private:
    ImmutableVector(std::shared_ptr<Node<T>> root, size_t size) : root_(root), size_(size) {
    }

    void Copy(const ImmutableVector& other) {
        root_ = other.root_;
        size_ = other.size_;
    }

    const T& GetValue(std::shared_ptr<Node<T>> v, size_t index, size_t batch_id,
                      size_t max_batch_id) const {
        if (batch_id == max_batch_id) {
            if (!v || !v->value) {
                throw std::runtime_error("Cannot find element with such index");
            }
            return v->value.value();
        }

        size_t bits = GetBitBatch(index, batch_id);
        if (!v) {
            throw std::runtime_error("Cannot find element with such index");
        }

        return GetValue(v->children[bits], index, batch_id + 1, max_batch_id);
    }

    size_t GetBitBatch(size_t index, size_t batch_id) const {
        size_t batch = (index >> (batch_id * kBitsInBatch));
        size_t mask = 0;
        for (size_t i = 0; i < kBitsInBatch; ++i) {
            mask |= (1 << i);
        }
        batch &= mask;
        return batch;
    }

    std::shared_ptr<Node<T>> Insert(std::shared_ptr<Node<T>> v, size_t index, size_t batch_id,
                                    size_t max_batch_id, const T& value) {
        if (batch_id == max_batch_id) {
            return std::make_shared<Node<T>>(value);
        }
        size_t bits = GetBitBatch(index, batch_id);
        std::vector<std::shared_ptr<Node<T>>> new_children = (*v).children;
        std::shared_ptr<Node<T>> tmp_node;
        if (!v->children[bits]) {
            tmp_node = std::make_shared<Node<T>>();
        } else {
            tmp_node = v->children[bits];
        }
        new_children[bits] = Insert(tmp_node, index, batch_id + 1, max_batch_id, value);
        return std::make_shared<Node<T>>(new_children, v->value);
    }

    std::shared_ptr<Node<T>> SetValue(std::shared_ptr<Node<T>> v, size_t index, size_t batch_id,
                                      size_t max_batch_id, const T& value) {
        if (batch_id == max_batch_id) {
            std::shared_ptr<Node<T>> new_node = std::make_shared<Node<T>>(v->children, v->value);
            new_node->value = value;
            return new_node;
        }
        size_t bits = GetBitBatch(index, batch_id);
        std::vector<std::shared_ptr<Node<T>>> new_children = (*v).children;
        std::shared_ptr<Node<T>> tmp_node;
        if (!v->children[bits]) {
            tmp_node = std::make_shared<Node<T>>();
        } else {
            tmp_node = v->children[bits];
        }
        new_children[bits] = SetValue(tmp_node, index, batch_id + 1, max_batch_id, value);
        return std::make_shared<Node<T>>(new_children, v->value);
    }

    size_t GetMaxBatchId(size_t batch) const {
        size_t res = 0;
        while (batch) {
            batch >>= kBitsInBatch;
            ++res;
        }
        return std::max(static_cast<size_t>(1), res);
    }

    std::shared_ptr<Node<T>> Erase(std::shared_ptr<Node<T>> v, size_t index, size_t batch_id,
                                   size_t max_batch_id) {
        if (batch_id == max_batch_id) {
            return std::shared_ptr<Node<T>>();
        }

        size_t bits = GetBitBatch(index, batch_id);
        std::vector<std::shared_ptr<Node<T>>> new_children = (*v).children;
        std::shared_ptr<Node<T>> tmp_node;
        if (!v->children[bits]) {
            tmp_node = std::make_shared<Node<T>>();
        } else {
            tmp_node = v->children[bits];
        }
        new_children[bits] = Erase(tmp_node, index, batch_id + 1, max_batch_id);
        return std::make_shared<Node<T>>(new_children, v->value);
    }

    std::shared_ptr<Node<T>> root_;
    size_t size_;
};
