#ifndef EVENTLOOP_DUMPUTILS_HEADER
#define EVENTLOOP_DUMPUTILS_HEADER

#include <memory>
#include <iomanip>
#include <sstream>
#include <tuple>
#include <exception>
#include <boost/optional.hpp>
#include <boost/type_traits.hpp>
#include <boost/io/ios_state.hpp>

#include <EventLoop/Dumpable.hpp>
#include <EventLoop/TypeName.hpp>

namespace EventLoop
{
    namespace DumpUtils
    {
        namespace Detail
        {
            class NullPtr { };

            std::ostream & operator << (std::ostream &out, const NullPtr &);

            template <size_t N>
            struct TupleOutputHelper
            {
                template <typename T>
                static void output(std::ostream &os, const T &t);
            };

            template <>
            struct TupleOutputHelper<0>
            {
                template <typename T>
                static void output(std::ostream &, const T &)
                {
                }
            };

            void byteOutput(std::ostream    &os,
                            const void      *ptr,
                            size_t          size);

            void outputString(std::ostream      &os,
                              const std::string &str);

            void outputString(std::ostream  &os,
                              const char    *str);

            bool outputValue(std::ostream       &os,
                             const std::string  &value);

            bool outputValue(std::ostream   &os,
                             const char     *value);

            bool outputValue(std::ostream           &os,
                             const std::exception   &e);

            template <typename V>
            bool outputValue(std::ostream               &os,
                             const V                    &value,
                             const boost::false_type    &,
                             const boost::false_type    &,
                             const boost::false_type    &)
            {
                byteOutput(os, &value, sizeof(value));
                return false;
            }

            template <typename V>
            bool outputValue(std::ostream               &os,
                             const V                    &value,
                             const boost::false_type    &,
                             const boost::false_type    &,
                             const boost::true_type     &)
            {
                os << static_cast<int>(value);
                return false;
            }

            template <typename V, typename Any1, typename Any2>
            bool outputValue(std::ostream           &os,
                             const V                &value,
                             const boost::true_type &,
                             const Any1             &,
                             const Any2             &)
            {
                value.dump(os);
                return true;
            }

            template <typename V>
            bool outputValue(std::ostream               &os,
                             const V                    &value,
                             const boost::false_type    &,
                             const boost::true_type     &,
                             const boost::false_type    &)
            {
                os << value;
                return false;
            }

            template <typename V>
            bool outputValue(std::ostream               &os,
                             const V                    &value,
                             const boost::false_type    &,
                             const boost::true_type     &,
                             const boost::true_type     &)
            {
                os << value << " (" << static_cast<int>(value) << ')';
                return false;
            }

            template <typename V>
            bool outputValue(std::ostream   &os,
                             const V        &value)
            {
                return outputValue(os, value,
                                   boost::is_base_of<EventLoop::Dumpable, V>(),
                                   boost::has_left_shift<std::ostream, V>(),
                                   boost::is_enum<V>());
            }

            template <typename V>
            bool outputValue(std::ostream           &os,
                             const std::function<V> &value)
            {
                return outputValue(os, static_cast<bool>(value));
            }

            template <typename V>
            bool outputValue(std::ostream               &os,
                             const boost::optional<V>   &value)
            {
                if (value)
                {
                    return outputValue(os, *value);
                }
                else
                {
                    os << "<none>";
                    return false;
                }
            }

            template <typename V>
            bool outputValue(std::ostream               &os,
                             const std::shared_ptr<V>   &value)
            {
                os << value.get() << " (use count: " << value.use_count() << ')';
                return false;
            }

            template <typename V>
            bool outputValue(std::ostream           &os,
                             const std::weak_ptr<V> &value)
            {
                return outputValue(os, value.lock());
            }

            template <typename V>
            bool outputValue(std::ostream               &os,
                             const std::unique_ptr<V>   &value)
            {
                return outputValue(os, value.get());
            }

            template <typename... T>
            bool outputValue(std::ostream           &os,
                             const std::tuple<T...> &t)
            {
                os << '(';
                TupleOutputHelper<std::tuple_size<std::tuple<T...>>::value>::output(os, t);
                os << ')';
                return false;
            }

            template <size_t N> template <typename T>
            void TupleOutputHelper<N>::output(std::ostream &os, const T &t)
            {
                TupleOutputHelper<N - 1>::output(os, t);
                if (N > 1)
                {
                    os << ", ";
                }
                outputValue(os, std::get<N - 1>(t));
            }

            void outputName(std::ostream    &os,
                            const char      *name);

            template <typename N>
            void outputName(std::ostream    &os,
                            const N         &name)
            {
                outputValue(os, name);
            }

            template <typename N, typename V>
            void outputNameAndValue(std::ostream    &os,
                                    const N         &name,
                                    const V         &value)
            {
                std::ostringstream tmp;
                tmp.flags(os.flags());
                if (!outputValue(tmp, value))
                {
                    outputName(os, name);
                    os << ": " << tmp.str() << '\n';
                }
                else
                {
                    outputName(os, name);
                    os << ":\n{\n" << tmp.str() << "}\n";
                }
            }

            template <typename V>
            void outputNamelessValue(std::ostream   &os,
                                     const V        &value)
            {
                std::ostringstream tmp;
                tmp.flags(os.flags());
                if (!outputValue(tmp, value))
                {
                    os << tmp.str() << '\n';
                }
                else
                {
                    os << tmp.str();
                }
            }

            template <typename N, typename V>
            void outputNameAndHexValue(std::ostream &os,
                                       const N      &name,
                                       const V      &value)
            {
                outputName(os, name);
                const boost::io::ios_flags_saver ifs(os);
                os << ": 0x" << std::hex;
                outputValue(os, value);
                os << '\n';
            }

            template <typename N, typename V>
            void outputNameAndValuePointedBy(std::ostream   &os,
                                             const N        &name,
                                             const V        &value)
            {
                if (value == nullptr)
                {
                    outputNameAndValue(os, name, NullPtr());
                }
                else
                {
                    outputNameAndValue(os, name, *value);
                }
            }

            template <typename N, typename V>
            void outputNameAndValuePointedBy(std::ostream               &os,
                                             const N                    &name,
                                             const std::shared_ptr<V>   &value)
            {
                if (value == nullptr)
                {
                    outputNameAndValue(os, name, NullPtr());
                }
                else
                {
                    std::ostringstream tmp;
                    tmp.flags(os.flags());
                    if (!outputValue(tmp, *value))
                    {
                        outputName(os, name);
                        os << ": " << tmp.str() << '\n';
                    }
                    else
                    {
                        outputName(os, name);
                        os << ":\n{\n" << tmp.str() << "use count: " << value.use_count() << "\n}\n";
                    }
                }
            }

            template <typename N, typename V>
            void outputNameAndValuePointedBy(std::ostream           &os,
                                             const N                &name,
                                             const std::weak_ptr<V> &value)
            {
                outputNameAndValuePointedBy(os, name, value.lock());
            }

            template <typename C>
            void outputContainer(std::ostream   &os,
                                 const char     *name,
                                 const C        &c)
            {
                os << name << ":\n{\n";
                for (const auto &i : c)
                {
                    outputNamelessValue(os, i);
                }
                os << "}\n";
            }

            template <typename C>
            void outputPtrContainer(std::ostream    &os,
                                    const char      *name,
                                    const C         &c)
            {
                os << name << ":\n{\n";
                for (const auto &i : c)
                {
                    if (i == nullptr)
                    {
                        os << "null\n";
                    }
                    else
                    {
                        outputNamelessValue(os, *i);
                    }
                }
                os << "}\n";
            }

            template <typename C>
            void outputPairContainer(std::ostream   &os,
                                     const char     *name,
                                     const C        &c)
            {
                os << name << ":\n{\n";
                for (const auto &i : c)
                {
                    outputNameAndValue(os, i.first, i.second);
                }
                os << "}\n";
            }

            template <typename C>
            void outputPtrPairContainer(std::ostream    &os,
                                        const char      *name,
                                        const C         &c)
            {
                os << name << ":\n{\n";
                for (const auto &i : c)
                {
                    if (i.second == nullptr)
                    {
                        outputNameAndValue(os, i.first, NullPtr());
                    }
                    else
                    {
                        outputNameAndValue(os, i.first, *i.second);
                    }
                }
                os << "}\n";
            }
        }
    }
}

#define DUMP_BEGIN(os) do { char _buffer[EVENTLOOP_MAX_TYPE_NAME]; (os) << EventLoop::getTypeName(*this, _buffer, sizeof(_buffer)) << "\n{\nthis: " << this << '\n'; const int _dontForgetToEndDump(0); {

#define DUMP_BLOCK_BEGIN(os, title) do { (os) << (title) << ":\n{\n"; const int _dontForgetToEndBlock(0); {

#define DUMP_BLOCK_END(os) } static_cast<void>(_dontForgetToEndBlock); (os) << "}\n"; } while (false)

#define DUMP_END(os) } static_cast<void>(_dontForgetToEndDump); (os) << "}\n"; } while (false)

#define DUMP_VALUE_OF(os, var) EventLoop::DumpUtils::Detail::outputNameAndValue((os), #var, (var))

#define DUMP_HEX_VALUE_OF(os, var) EventLoop::DumpUtils::Detail::outputNameAndHexValue((os), #var, (var))

#define DUMP_ADDRESS_OF(os, var) EventLoop::DumpUtils::Detail::outputNameAndValue((os), #var, static_cast<const void *>(&(var)))

#define DUMP_VALUE_POINTED_BY(os, ptr) EventLoop::DumpUtils::Detail::outputNameAndValuePointedBy((os), #ptr, (ptr))

#define DUMP_RESULT_OF(os, func)                                        \
    do                                                                  \
    {                                                                   \
        try                                                             \
        {                                                               \
            EventLoop::DumpUtils::Detail::outputNameAndValue((os), #func, func); \
        }                                                               \
        catch (const std::exception &e)                                 \
        {                                                               \
            EventLoop::DumpUtils::Detail::outputNameAndValue((os), #func, e); \
        }                                                               \
    }                                                                   \
    while (false)

#define DUMP_AS(os, type) do { auto &_autoos(os); DUMP_BLOCK_BEGIN(_autoos, "parent"); type::dump(_autoos); DUMP_BLOCK_END(_autoos); } while (false)

#define DUMP_CONTAINER(os, c) EventLoop::DumpUtils::Detail::outputContainer((os), #c, (c))

#define DUMP_PTR_CONTAINER(os, c) EventLoop::DumpUtils::Detail::outputPtrContainer((os), #c, (c))

#define DUMP_PAIR_CONTAINER(os, pc) EventLoop::DumpUtils::Detail::outputPairContainer((os), #pc, (pc))

#define DUMP_PTR_PAIR_CONTAINER(os, pc) EventLoop::DumpUtils::Detail::outputPtrPairContainer((os), #pc, (pc))

#endif /* !EVENTLOOP_DUMPUTILS_HEADER */
