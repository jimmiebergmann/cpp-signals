/*
* MIT License
*
* Copyright(c) 2018 Jimmie Bergmann
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

#ifndef CPP_SIGNALS__HPP
#define CPP_SIGNALS__HPP

#include <functional>
#include <memory>
#include <mutex>
#include <map>
#include <set>

// Forward declarations.
class Slot;
class Connection;
template<typename... Args> class Signal;


class SignalBase
{

public:

    virtual void disconnect(std::shared_ptr<Connection> connection) = 0;

};


class Connection
{

public:

    void disconnect()
    {
        std::lock_guard<std::recursive_mutex> sm(m_mutex);
        if (m_pSignal)
        {
            SignalBase * pTmpSignal = m_pSignal;
            std::shared_ptr<Connection> tempShared(this, [](Connection *) {});
            pTmpSignal->disconnect(tempShared);
        }
    }

private:

    template<typename... Args> friend class Signal;
    friend class Slot;

    Connection(SignalBase * signal, Slot * slot) :
        m_pSignal(signal),
        m_pSlot(slot) {};

    SignalBase * m_pSignal;
    Slot * m_pSlot;
    std::recursive_mutex m_mutex;

};


class Slot
{

public:

    virtual ~Slot()
    {
        std::lock_guard<std::recursive_mutex> sm(m_mutex);

        for (auto & conn : m_connections)
        {
            std::lock_guard<std::recursive_mutex> conn_sm(conn->m_mutex);
            if (conn->m_pSignal)
            {
                conn->m_pSlot = nullptr;
                SignalBase * pTempSignal = conn->m_pSignal;
                conn->m_pSignal = nullptr;
                pTempSignal->disconnect(conn);
            }
        }
    }

private:

    template<typename... Args> friend class Signal;

    std::set<std::shared_ptr<Connection>> m_connections;
    std::recursive_mutex m_mutex;

};


template<typename... Args>
class Signal : public SignalBase
{

public:

    ~Signal()
    {
        disconnectAll();
    }

    template<typename T>
    std::shared_ptr<Connection> connect(T * slot, void(T::* function)(Args...))
    {
        static_assert(std::is_base_of<Slot, T>::value, "T is not base of Slot.");

        std::lock_guard<std::mutex> sm(m_mutex);
        std::lock_guard<std::recursive_mutex> slot_sm(slot->m_mutex);
        std::shared_ptr<Connection> connection(new Connection(this, slot));
        m_connections.insert({ connection, [slot, function](Args... args)
        {
            (slot->*function)(args...);
        }
            });
        slot->m_connections.insert(connection);
        return connection;
    }

    std::shared_ptr<Connection> connect(std::function<void(Args...)> function)
    {
        std::lock_guard<std::mutex> sm(m_mutex);
        std::shared_ptr<Connection> connection(new Connection(this, nullptr));
        m_connections.insert({ connection, function });
        return connection;
    }

    virtual void disconnect(std::shared_ptr<Connection> connection)
    {
        std::lock_guard<std::mutex> sm(m_mutex);
        if (m_connections.erase(connection) == 0)
        {
            return;
        }

        std::lock_guard<std::recursive_mutex> conn_sm(connection->m_mutex);
        connection->m_pSignal = nullptr;
        Slot * pSlot = connection->m_pSlot;

        if (pSlot)
        {
            std::lock_guard<std::recursive_mutex> slot_sm(pSlot->m_mutex);
            pSlot->m_connections.erase(connection);
            connection->m_pSlot = nullptr;
        }
    }

    void disconnectAll()
    {
        std::lock_guard<std::mutex> sm(m_mutex);
        for (auto & kv : m_connections)
        {
            auto conn = kv.first;
            std::lock_guard<std::recursive_mutex> conn_sm(conn->m_mutex);
            conn->m_pSignal = nullptr;

            Slot * pSlot = conn->m_pSlot;
            if (pSlot)
            {

                std::lock_guard<std::recursive_mutex> sm(pSlot->m_mutex);
                pSlot->m_connections.erase(conn);
            }
        }
        m_connections.clear();
    }

    void emit(Args ... args)
    {
        std::lock_guard<std::mutex> sm(m_mutex);
        for (auto & kv : m_connections)
        {
            kv.second(args...);
        }
    }

    /**
    * Same as emit(...)
    *
    * @param args Passed signal arguments.
    *
    */
    void operator()(Args ... args)
    {
        std::lock_guard<std::mutex> sm(m_mutex);
        for (auto & kv : m_connections)
        {
            kv.second(args...);
        }
    }

    size_t count()
    {
        std::lock_guard<std::mutex> sm(m_mutex);
        return m_connections.size();
    }

private:

    std::map<std::shared_ptr<Connection>, std::function<void(Args...)>> m_connections;
    std::mutex m_mutex;

};

#endif
