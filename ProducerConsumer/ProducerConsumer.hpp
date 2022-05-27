//
//  ProducerConsumer.hpp
//  ProducerConsumer
//
//  Created by Pengliang Yong on 2022/5/26.
//

#ifndef ProducerConsumer_hpp
#define ProducerConsumer_hpp

#ifdef __APPLE__

#include <dispatch/dispatch.h>
typedef dispatch_semaphore_t semaphore_t;
#define semaphore_init(sem, value)                  (sem = dispatch_semaphore_create(value), sem ? 0 : -1)
#define semaphore_signal(sem)                       dispatch_semaphore_signal(sem)
#define semaphore_wait(sem, timeout)                dispatch_semaphore_wait(sem, timeout)
#define semaphore_lock(sem)                         dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER)
#define semaphore_unlock(sem)                       dispatch_semaphore_signal(sem)

typedef dispatch_time_t semaphore_time_t;

static inline semaphore_time_t SEMAPHORE_TIME(double timeout) {
    return  dispatch_time(DISPATCH_TIME_NOW, (int64_t)(timeout * NSEC_PER_SEC));
}

static inline semaphore_time_t SEMAPHORE_TIME_FOREVER() {
    return DISPATCH_TIME_FOREVER;
}

#elif __ANDROID__ || __linux__

#include <semaphore.h>
#include <time.h>
#include <limits.h>
typedef sem_t semaphore_t;
#define semaphore_init(sem, value)                  sem_init(&sem, 0, value)
#define semaphore_signal(sem)                       sem_post(&sem)
#define semaphore_wait(sem, timeout)                sem_timedwait(&sem, &timeout)
#define semaphore_lock(sem)                         sem_post(&sem)
#define semaphore_unlock(sem)                       sem_wait(&sem)

#define NSEC_PER_SEC 1000000000ull

typedef struct timespec semaphore_time_t;

static inline semaphore_time_t SEMAPHORE_TIME_FOREVER() {
    semaphore_time_t st = {LONG_MAX, LONG_MAX};
    return st;
}

static inline semaphore_time_t SEMAPHORE_TIME(double timeout) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts)){
        return {LONG_MAX, LONG_MAX};
    }
    unsigned long long nsec = ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec + (unsigned long long)(timeout * NSEC_PER_SEC);
    ts.tv_sec = nsec / NSEC_PER_SEC;
    ts.tv_nsec = nsec % NSEC_PER_SEC;
    return ts;
}

#endif

#define SEMAPHORE_TIMEOUT_FOREVER    (100.0 * 365 * 24 * 3600)

namespace YPL {

template <class ProductType>
class SimplCStructCircleQueue {

protected:
    ProductType *_productArray = nullptr;
    unsigned _capacity = 0;
    unsigned _tail = 0;
    unsigned _header = 0;
    bool _initSuccess = false;

public:
    SimplCStructCircleQueue(unsigned capacity) {
        _productArray = (ProductType *)calloc(capacity, sizeof(ProductType));
        if (!_productArray) {
            return;
        }
        _capacity = capacity;
        _initSuccess = true;
    }

    virtual ~SimplCStructCircleQueue() {
        if (_productArray) {
            free(_productArray);
            _productArray = nullptr;
        }
    }

    bool initSuccess() {
        return _initSuccess;
    }

    unsigned size() {
        return _tail - _header;
    }

    bool isFull() {
        return (_tail - _header) % _capacity == 0 && _tail != _header;
    }

    bool isEmpty() {
        return _tail == _header;
    }

    bool enqueue(ProductType &product) {
        if (isFull()) {
            return false;
        }
        _productArray[_tail % _capacity] = product;
        _tail++;
        return true;
    }

    bool enqueue(ProductType &&product) {
        enqueue((ProductType &)product);
    }

    bool dequeue(ProductType *product) {
        if (isEmpty()) {
            return false;
        }
        unsigned index = _header % _capacity;
        if (product) {
            *product = _productArray[index];
        }
        _header++;
        return true;
    }
};


template <class ProductType>
class SimpleObjectCircleQueue : public SimplCStructCircleQueue<ProductType> {

public:
    SimpleObjectCircleQueue(unsigned capacity) : SimplCStructCircleQueue<ProductType>(capacity) { }

    bool enqueue(ProductType &product) {
        if (this->isFull()) {
            return false;
        }
        new (&this->_productArray[this->_tail % this->_capacity]) ProductType(product);
        this->_tail++;
        return true;
    }

    bool enqueue(ProductType &&product) {
        if (this->isFull()) {
            return false;
        }
        new (&this->_productArray[this->_tail % this->_capacity]) ProductType(static_cast<ProductType&&>(product));
        this->_tail++;
        return true;
    }

    bool dequeue(ProductType *product) {
        if (this->isEmpty()) {
            return false;
        }
        unsigned index = this->_header % this->_capacity;
        if (product) {
            *product = static_cast<ProductType&&>(this->_productArray[index]);
        }
        (&this->_productArray[index])->~ProductType();
        this->_header++;
        return true;
    }
};


template <class ProductType, class CircleQueue = SimplCStructCircleQueue<ProductType>>
class ProducerConsumer {

public:
    typedef enum {
        ProducerConsumerBlcokTypeNoneBlock      = 0,
        ProducerConsumerBlcokTypeProducerBlockWhenBufferIsFull  = 0x1,
        ProducerConsumerBlcokTypeConsumerBlockWhenBufferIsEmpty  = 0x1 << 1,
        ProducerConsumerBlcokTypeAllBlock       = ProducerConsumerBlcokTypeProducerBlockWhenBufferIsFull | ProducerConsumerBlcokTypeConsumerBlockWhenBufferIsEmpty
    } ProducerConsumerBlcokType;

private:
    CircleQueue *_queue = nullptr;
    semaphore_t _fullSem;
    semaphore_t _emptySem;
    semaphore_t _lock;
    ProducerConsumerBlcokType _blockType;
    bool _stopProduce = false;

    ProducerConsumer() {}

public:
    ~ProducerConsumer() {
        stopProduce();
        if (_queue) {
            semaphore_lock(_lock);
            unsigned size = _queue->size();
            for (unsigned i = 0; i < size; i++) {
                semaphore_time_t ts = SEMAPHORE_TIME_FOREVER();
                semaphore_wait(_fullSem, ts);
                _queue->dequeue(nullptr);
                semaphore_signal(_emptySem);
            }
            delete _queue;
            _queue = nullptr;
            semaphore_unlock(_lock);
        }
    }

    static ProducerConsumer* Init(unsigned capacity, ProducerConsumerBlcokType blockType = ProducerConsumerBlcokTypeAllBlock) {
        ProducerConsumer *instance = new ProducerConsumer();
        if (!instance) {
            return nullptr;
        }
        if (semaphore_init(instance->_fullSem, 0) || semaphore_init(instance->_emptySem, capacity) || semaphore_init(instance->_lock, 1)) {
            delete instance;
            return nullptr;
        }
        auto queue = new CircleQueue(capacity);
        if (!queue || !queue->initSuccess()) {
            delete instance;
            if (queue) delete queue;
            return nullptr;
        }
        instance->_queue = queue;
        instance->_blockType = blockType;
        return instance;
    }

    void stopProduce(bool arg = true) {
        _stopProduce = arg;
    }
    
    bool produce(ProductType &product, double timeout = SEMAPHORE_TIMEOUT_FOREVER) {
        return  produce(product, timeout, _blockType & ProducerConsumerBlcokTypeProducerBlockWhenBufferIsFull);
    }
    
    bool produce(ProductType &product, double timeout, bool bockWhenBufferIsFull) {
        if (produceShouldRetunFalse(bockWhenBufferIsFull)) {
            return false;
        }
        semaphore_time_t st = SEMAPHORE_TIME(timeout);
        if (semaphore_wait(_emptySem, st)) {
            return false;
        }
        semaphore_lock(_lock);
        if (!_queue) {
            semaphore_unlock(_lock);
            semaphore_signal(_emptySem);
            return false;
        }
        bool retValue = _queue->enqueue(product);
        semaphore_unlock(_lock);
        semaphore_signal(_fullSem);
        return retValue;
    }
    
    bool produce(ProductType &&product, double timeout = SEMAPHORE_TIMEOUT_FOREVER) {
        return produce(static_cast<ProductType&&>(product), timeout, _blockType & ProducerConsumerBlcokTypeProducerBlockWhenBufferIsFull);
    }

    bool produce(ProductType &&product, double timeout, bool bockWhenBufferIsFull) {
        if (produceShouldRetunFalse(bockWhenBufferIsFull)) {
            return false;
        }
        semaphore_time_t st = SEMAPHORE_TIME(timeout);
        if (semaphore_wait(_emptySem, st)) {
            return false;
        }
        semaphore_lock(_lock);
        if (!_queue) {
            semaphore_unlock(_lock);
            semaphore_signal(_emptySem);
            return false;
        }
        bool retValue = _queue->enqueue(static_cast<ProductType&&>(product));
        semaphore_unlock(_lock);
        semaphore_signal(_fullSem);
        return retValue;
    }
    
    
    bool consume(ProductType *product,  double timeout = SEMAPHORE_TIMEOUT_FOREVER) {
        return consume(product, timeout,  _blockType & ProducerConsumerBlcokTypeConsumerBlockWhenBufferIsEmpty);
    }

    bool consume(ProductType *product,  double timeout, bool bockWhenBufferIsEmpty) {
        if (consumeShouldRetunFalse(bockWhenBufferIsEmpty)) {
            return false;
        }
        semaphore_time_t st = SEMAPHORE_TIME(timeout);
        if (semaphore_wait(_fullSem, st)) {
            return false;
        }
        semaphore_lock(_lock);
        if (!_queue) {
            semaphore_unlock(_lock);
            semaphore_signal(_fullSem);
            return false;
        }
        bool retValue = _queue->dequeue(product);
        semaphore_unlock(_lock);
        semaphore_signal(_emptySem);
        return retValue;
    }

    bool isEmpty(bool *queueIsNull = nullptr) {
        semaphore_lock(_lock);
        if (!_queue) {
            semaphore_unlock(_lock);
            if (queueIsNull) *queueIsNull = true;
            return false;
        }
        bool isEmpty = _queue->isEmpty();
        if (queueIsNull) *queueIsNull = false;
        semaphore_unlock(_lock);
        return isEmpty;
    }

    bool isFull(bool *queueIsNull = nullptr) {
        semaphore_lock(_lock);
        if (!_queue) {
            semaphore_unlock(_lock);
            if (queueIsNull) *queueIsNull = true;
            return false;
        }
        bool isFull = _queue->isFull();
        if (queueIsNull) *queueIsNull = false;
        semaphore_unlock(_lock);
        return isFull;
    }

    unsigned size(bool *queueIsNull = nullptr) {
        semaphore_lock(_lock);
        if (!_queue) {
            semaphore_unlock(_lock);
            if (queueIsNull) *queueIsNull = true;
            return 0;
        }
        unsigned size = _queue->size();
        if (queueIsNull) *queueIsNull = false;
        semaphore_unlock(_lock);
        return size;
    }

private:
    bool produceShouldRetunFalse(bool bockWhenBufferIsFull) {
        if (_stopProduce ) {
            return true;
        }
        
        bool queueIsNull = false;
        if (!bockWhenBufferIsFull) {
            bool isFull = this->isFull(&queueIsNull);
            return isFull || queueIsNull;
        }
        return false;
    }

    bool consumeShouldRetunFalse(bool bockWhenBufferIsEmpty) {
        bool queueIsNull = false;
        if (!bockWhenBufferIsEmpty) {
            bool isEmpty = this->isEmpty(&queueIsNull);
            return isEmpty || queueIsNull;
        }
        return false;
    }
};

}

#endif /* ProducerConsumer_hpp */

