//
//  main.cpp
//  ProducerConsumer
//
//  Created by Pengliang Yong on 2022/5/26.
//

#include <string>
#include <utility>
#include <sstream>
#include "ProducerConsumer.hpp"


#if __ANDROID__
#include <jni.h>
#include <android/log.h>

class EndLine {
};

EndLine endLine;

class ConsoleOut {

public:
    std::ostringstream stream;
};

EndLine endl;
ConsoleOut cout;

template <class T>
ConsoleOut& operator<< (ConsoleOut& co, T el)  {
    co.stream << el;
    return co;
}

ConsoleOut& operator<< (ConsoleOut& co, EndLine &el) {
    __android_log_write(ANDROID_LOG_INFO, "your tag", co.stream.str().c_str());
    co.stream.str("");
    return co;
}

#else

#include <iostream>
using namespace std;

#endif

using namespace YPL;


typedef struct{
    char buffer[1024];
    size_t buffer_size;
} Buffer;

void Buffer_init(Buffer &buffer, const char *buffer_content) {
    if (!buffer_content) {
        buffer.buffer_size = 0;
    } else {
        strcpy(buffer.buffer, buffer_content);
        buffer.buffer_size = strlen(buffer_content);
    }
}


class Person {
public:
    const char *name = nullptr;
    int age = 0;

    Person& operator = (Person &p) {
        cout << "operator = (Person &p) :" << this << endl;
        name = strdup(p.name);
        age = p.age;
        return (*this);
    }

    Person& operator = (Person &&p) {
        cout << "operator = (Person &&p) :" << this << endl;
        name = p.name;
        age = p.age;
        p.name = nullptr;
        p.age = 0;
        return (*this);
    }

    Person(Person &p) {
        cout << "Person(Person &p) :" << this << endl;
        name = strdup(p.name);
        age = p.age;
    }

    Person(Person &&p) {
        cout << "Person(Person &&p) :" << this << endl;
        name = p.name;
        age = p.age;
        p.name = nullptr;
        p.age = 0;
    }

    Person(const char *name, int age) {
        cout << "Person(char *name, int age) :" << this << endl;
        this->name = strdup(name);
        this->age = age;
    }
    Person() {}

    ~Person() {
        cout << "~Person() :" << this << endl;
        if (name) {
            free((void *)name);
            name = nullptr;
        }
    }

};


void testCStructQueue(void) {
    Buffer b1 = {0};
    Buffer_init(b1, "buffer content1");

    Buffer b2 = {0};
    Buffer_init(b2, "buffer content2");

    Buffer b3 = {0};
    Buffer_init(b3, "buffer content3");

    Buffer b4 = {0};
    Buffer_init(b4, "buffer content4");

    Buffer b5 = {0};
    Buffer_init(b5, "buffer content5");

    Buffer b6 = {0};
    Buffer_init(b6, "buffer content6");

    Buffer b7 = {0};
    Buffer_init(b7, "buffer content3");


    auto producerConsumerBuffer = ProducerConsumer<Buffer>::Init(5);

    bool b = producerConsumerBuffer->produce(b1);
    cout << "produce result: " << b << ", queque size: " << producerConsumerBuffer->size() << endl;
    b = producerConsumerBuffer->produce(b2);
    cout << "produce result: " << b << ", queque size: " << producerConsumerBuffer->size() << endl;
    b = producerConsumerBuffer->produce(b3);
    cout << "produce result: " << b << ", queque size: " << producerConsumerBuffer->size() << endl;
    b = producerConsumerBuffer->produce(b4);
    cout << "produce result: " << b << ", queque size: " << producerConsumerBuffer->size() << endl;
    b = producerConsumerBuffer->produce(b5);
    cout << "produce result: " << b << ", queque size: " << producerConsumerBuffer->size() << endl;
    b = producerConsumerBuffer->produce(b6, 2);
    cout << "produce result: " << b << ", queque size: " << producerConsumerBuffer->size() << endl;
    b = producerConsumerBuffer->produce(b7, 2);
    cout << "produce result: " << b << ", queque size: " << producerConsumerBuffer->size() << endl;

    cout << "-------------------------------------------------------" << endl;

    Buffer outbuf;
    b = producerConsumerBuffer->consume(&outbuf);
    cout << "consume result: " << b << ", queque size: " << producerConsumerBuffer->size() << ", buffer: " << outbuf.buffer  << endl;

    b = producerConsumerBuffer->consume(&outbuf);
    cout << "consume result: " << b << ", queque size: " << producerConsumerBuffer->size() << ", buffer: " << outbuf.buffer  << endl;

    b = producerConsumerBuffer->consume(&outbuf);
    cout << "consume result: " << b << ", queque size: " << producerConsumerBuffer->size() << ", buffer: " << outbuf.buffer  << endl;

    b = producerConsumerBuffer->consume(&outbuf);
    cout << "consume result: " << b << ", queque size: " << producerConsumerBuffer->size() << ", buffer: " << outbuf.buffer  << endl;

    b = producerConsumerBuffer->consume(&outbuf);
    cout << "consume result: " << b << ", queque size: " << producerConsumerBuffer->size() << ", buffer: " << outbuf.buffer  << endl;

    b = producerConsumerBuffer->consume(&outbuf, 2);
    cout << "consume result: " << b << ", queque size: " << producerConsumerBuffer->size() << ", buffer: " << outbuf.buffer  << endl;

    b = producerConsumerBuffer->consume(&outbuf, 1);
    cout << "consume result: " << b << ", queque size: " << producerConsumerBuffer->size() << ", buffer: " << outbuf.buffer  << endl;
}

void testObjectQueue() {

    Person p1 = Person("zhagnsan1", 1);
    Person p2 = Person("zhagnsan2", 2);
    Person p3 = Person("zhagnsan3", 3);
    Person p4 = Person("zhagnsan4", 4);
    Person p5 = Person("zhagnsan5", 5);
    Person p6 = Person("zhagnsan6", 6);
    Person p7 = Person("zhagnsan7", 7);



    auto producerConsumerPerson = ProducerConsumer<Person, SimpleObjectCircleQueue<Person>>::Init(5);

    bool b = producerConsumerPerson->produce(std::move(p1));
    cout << "produce result: " << b << ", queque size: " << producerConsumerPerson->size() << endl;
    b = producerConsumerPerson->produce(std::move(p2));
    cout << "produce result: " << b << ", queque size: " << producerConsumerPerson->size() << endl;
    b = producerConsumerPerson->produce(std::move(p3));
    cout << "produce result: " << b << ", queque size: " << producerConsumerPerson->size() << endl;
    b = producerConsumerPerson->produce(std::move(p4));
    cout << "produce result: " << b << ", queque size: " << producerConsumerPerson->size() << endl;
    b = producerConsumerPerson->produce(std::move(p5));
    cout << "produce result: " << b << ", queque size: " << producerConsumerPerson->size() << endl;
    b = producerConsumerPerson->produce(std::move(p6), 2);
    cout << "produce result: " << b << ", queque size: " << producerConsumerPerson->size() << endl;
    b = producerConsumerPerson->produce(std::move(p7), 2);
    cout << "produce result: " << b << ", queque size: " << producerConsumerPerson->size() << endl;

    cout << "---------------------------------------------------" << endl;

    Person outPerson;
    b = producerConsumerPerson->consume(&outPerson);
    cout << "consume result: " << b << ", queque size: " << producerConsumerPerson->size() << ", name: " << outPerson.name << endl;

    b = producerConsumerPerson->consume(&outPerson);
    cout << "consume result: " << b << ", queque size: " << producerConsumerPerson->size() << ", name: " << outPerson.name  << endl;

    b = producerConsumerPerson->consume(&outPerson);
    cout << "consume result: " << b << ", queque size: " << producerConsumerPerson->size() << ", name: " << outPerson.name  << endl;

    b = producerConsumerPerson->consume(&outPerson);
    cout << "consume result: " << b << ", queque size: " << producerConsumerPerson->size() << ", name: " << outPerson.name  << endl;

    b = producerConsumerPerson->consume(&outPerson);
    cout << "consume result: " << b << ", queque size: " << producerConsumerPerson->size() << ", name: " << outPerson.name  << endl;

    b = producerConsumerPerson->consume(&outPerson, 2);
    cout << "consume result: " << b << ", queque size: " << producerConsumerPerson->size() << ", name: " << outPerson.name  << endl;

    b = producerConsumerPerson->consume(&outPerson, 4);
    cout << "consume result: " << b << ", queque size: " << producerConsumerPerson->size() << ", name: " << outPerson.name  << endl;
}


int main(int argc, const char * argv[]) {
    cout << "=================Test C Struct Buffer Queue Begin   ======================" << endl;
    testCStructQueue();
    cout << "=================Test C Struct Buffer Queue End     ======================" << endl;
    cout << endl;
    cout << "=================Test C++ Object Buffer Queue Begin ======================" << endl;
    testObjectQueue();
    cout << "=================Test C++ Object Buffer Queue End   ======================" << endl;
    
    return 0;
}
