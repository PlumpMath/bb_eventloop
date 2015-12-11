#include <exception>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <EventLoop/Runner.hpp>

#include "private/FDListenerMap.hpp"
#include "private/tst/DumpTest.hpp"
#include "private/tst/FDListenerMock.hpp"

using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    void dummyCallback(FDListener &, int, unsigned int) { }

    class WrongCallback: public std::exception { };

    void throwCallback(FDListener &, int, unsigned int) { throw WrongCallback(); }

    struct CallbackData
    {
        FDListener *fl;
        int fd;
        unsigned int events;

        CallbackData(): fl(nullptr), fd(0), events(0) { }
    };

    void setDataCallback(CallbackData &cbd, FDListener &fl, int fd, unsigned int events)
    {
        cbd.fl = &fl;
        cbd.fd = fd;
        cbd.events = events;
    }
}

TEST(FDListenerMap, addAndGet)
{
    CallbackData cbd;
    FDListenerMap flm(std::bind(setDataCallback, std::ref(cbd), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), throwCallback, throwCallback);
    FDListenerMock fl;
    flm.add(fl, 1, FDListener::EVENT_IN | FDListener::EVENT_OUT);
    EXPECT_EQ(&fl, &flm.get(1));
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, flm.getEvents(1));
    EXPECT_EQ(&fl, cbd.fl);
    EXPECT_EQ(1, cbd.fd);
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, cbd.events);
}

TEST(FDListenerMap, doubleAddThrows)
{
    FDListenerMap flm(dummyCallback, throwCallback, throwCallback);
    FDListenerMock fl1;
    FDListenerMock fl2;
    flm.add(fl1, 1, 0);
    EXPECT_THROW(flm.add(fl2, 1, 0), Runner::FDAlreadyAdded);
}

TEST(FDListenerMap, delExisting)
{
    CallbackData cbd;
    FDListenerMap flm(dummyCallback, throwCallback, std::bind(setDataCallback, std::ref(cbd), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    FDListenerMock fl;
    flm.add(fl, 1, FDListener::EVENT_IN | FDListener::EVENT_OUT);
    flm.del(1);
    EXPECT_THROW(flm.get(1), Runner::NonExistingFDListener);
    EXPECT_EQ(&fl, cbd.fl);
    EXPECT_EQ(1, cbd.fd);
    EXPECT_EQ(FDListener::EVENT_IN | FDListener::EVENT_OUT, cbd.events);
}

TEST(FDListenerMap, delNonExistingThrows)
{
    FDListenerMap flm(throwCallback, throwCallback, dummyCallback);
    EXPECT_THROW(flm.del(1), Runner::NonExistingFDListener);
}

TEST(FDListenerMap, modifyExisting)
{
    CallbackData cbd;
    FDListenerMap flm(dummyCallback, std::bind(setDataCallback, std::ref(cbd), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), throwCallback);
    FDListenerMock fl;
    flm.add(fl, 1, 0);
    flm.modify(1, FDListener::EVENT_IN);
    EXPECT_EQ(&fl, &flm.get(1));
    EXPECT_EQ(FDListener::EVENT_IN, flm.getEvents(1));
    EXPECT_EQ(&fl, cbd.fl);
    EXPECT_EQ(1, cbd.fd);
    EXPECT_EQ(FDListener::EVENT_IN, cbd.events);
}

TEST(FDListenerMap, modifyExistingWithSameEventsDoesntCallCallback)
{
    FDListenerMap flm(dummyCallback, throwCallback, throwCallback);
    FDListenerMock fl;
    flm.add(fl, 1, FDListener::EVENT_IN);
    flm.modify(1, FDListener::EVENT_IN);
    EXPECT_EQ(&fl, &flm.get(1));
    EXPECT_EQ(FDListener::EVENT_IN, flm.getEvents(1));
}

TEST(FDListenerMap, modifyNonExistingThrows)
{
    FDListenerMap flm(throwCallback, dummyCallback, throwCallback);
    EXPECT_THROW(flm.modify(1, 0), Runner::NonExistingFDListener);
}

TEST(FDListenerMap, getNonExistingThrows)
{
    FDListenerMap flm(throwCallback, throwCallback, throwCallback);
    EXPECT_THROW(flm.get(1), Runner::NonExistingFDListener);
    EXPECT_THROW(flm.getEvents(1), Runner::NonExistingFDListener);
}

TEST(FDListenerMap, dump)
{
    FDListenerMap flm(throwCallback, throwCallback, throwCallback);
    EXPECT_VALID_DUMP(flm);
}
