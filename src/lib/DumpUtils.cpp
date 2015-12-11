#include <EventLoop/DumpUtils.hpp>

#include <vector>
#include <string>
#include <cctype>
#include <boost/regex.hpp>
#include <boost/io/ios_state.hpp>

namespace
{
    boost::regex REGEX("void[[:space:]](.+)(?=::dump\\()");

    void outputAsHexEscapeSequence(std::ostream &os,
                                   char         c)
    {
        const boost::io::ios_flags_saver ifs(os);
        os << "\\x" << std::hex << std::uppercase
           << static_cast<unsigned int>(static_cast<unsigned char>(c));
    }

    void outputAsCharLiteral(std::ostream   &os,
                             char           c)
    {
        switch (c)
        {
            case ('\0'): os << "\\0"; break;
            case ('\f'): os << "\\f"; break;
            case ('\n'): os << "\\n"; break;
            case ('\r'): os << "\\r"; break;
            case ('\t'): os << "\\t"; break;
            case ('\v'): os << "\\v"; break;
            case ('\''): os << "\\\'"; break;
            case ('\"'): os << "\\\""; break;
            case ('\\'): os << "\\\\"; break;
            default:
                if (std::isprint(c)) os << c;
                else outputAsHexEscapeSequence(os, c);
                break;
        }
    }
}

std::ostream & EventLoop::DumpUtils::Detail::operator << (std::ostream &out,
                                                          const EventLoop::DumpUtils::Detail::NullPtr &)
{
    return out << "null";
}

void EventLoop::DumpUtils::Detail::byteOutput(std::ostream  &os,
                                              const void    *ptr,
                                              size_t        size)
{
    const boost::io::ios_flags_saver ifs(os);
    os << std::hex;
    const unsigned char *c(static_cast<const unsigned char *>(ptr));
    for (size_t i = 0; i < size; ++i)
    {
        os << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(c[i]);
    }
}

void EventLoop::DumpUtils::Detail::outputString(std::ostream        &os,
                                                const std::string   &str)
{
    os << '\"';
    for (auto i : str)
    {
        outputAsCharLiteral(os, i);
    }
    os << '\"';
}

void EventLoop::DumpUtils::Detail::outputString(std::ostream    &os,
                                                const char      *str)
{
    if (str == nullptr)
    {
        os << EventLoop::DumpUtils::Detail::NullPtr();
    }
    else
    {
        os << '\"';
        for (const char *p = str ; *p != '\0'; ++p)
        {
            outputAsCharLiteral(os, *p);
        }
        os << '\"';
    }
}

void EventLoop::DumpUtils::Detail::outputName(std::ostream  &os,
                                              const char    *name)
{
    if (name == nullptr)
    {
        os << EventLoop::DumpUtils::Detail::NullPtr();
    }
    else
    {
        os << name;
    }
}

bool EventLoop::DumpUtils::Detail::outputValue(std::ostream         &os,
                                               const std::string    &value)
{
    EventLoop::DumpUtils::Detail::outputString(os, value);
    return false;
}

bool EventLoop::DumpUtils::Detail::outputValue(std::ostream &os,
                                               const char   *value)
{
    EventLoop::DumpUtils::Detail::outputString(os, value);
    return false;
}

bool EventLoop::DumpUtils::Detail::outputValue(std::ostream         &os,
                                               const std::exception &e)
{
    char buffer[EVENTLOOP_MAX_TYPE_NAME];
    os << getTypeName(e, buffer, sizeof(buffer)) << ": " << e.what();
    return false;
}
