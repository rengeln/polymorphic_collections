////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2012 Robert Engeln (engeln@gmail.com)
//  See accompanying LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYMORPHIC_COLLECTIONS_POLICY_HPP
#define POLYMORPHIC_COLLECTIONS_POLICY_HPP

#include <boost/thread/mutex.hpp>

namespace polymorphic_collections
{
    class no_lock
    {
    protected:
        bool lock()
        {
            return true;
        }

        void unlock()
        {
        }
    };

    class atomic
    {
    protected:
        typedef boost::mutex mutex_type;

        bool lock()
        {
            m_mutex.lock();
            return true;
        }

        void unlock()
        {
            m_mutex.unlock();
        }

    private:
        mutex_type m_mutex;
    };

    class atomic_nonblocking
    {
    protected:
        typedef boost::mutex mutex_type;

        bool lock()
        {
            return m_mutex.try_lock();
        }

        void unlock()
        {
            m_mutex.unlock();
        }

    private:
        mutex_type m_mutex;
    };
}

#endif  // POLYMORPHIC_COLLECTIONS_POLICY_HPP
