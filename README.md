# protopug

Small header-only library to serialize/parse C++ struct to/from protobuf wire format.

Usage:
```cpp

#include "protopug/protopug.h"

struct Message
{
    int32_t c;
    std::vector<int32_t> r;
    std::map<int32_t, int32_t> m;
    std::variant<int32_t, int64_t> v;
    std::optional<std::string> os;
};

namespace protopug
{
    template<>
    struct descriptor<Message>
    {
        static auto type()
        {
            return message(
                        field<1, &Message::c>("c"),
                        field<2, &Message::r>("r"),
                        map_field<3, &Message::m>("m"),
                        oneof_field<4, 0, &Message::v>("v_32"),
                        oneof_field<5, 1, &Message::v>("v_64"),
                        field<6, &Message::os>("os")
                   );
        }
    };
}

int main()
{
    Message m{};
    m.c = 1;
    m.r.push_back(123);
    m.r.push_back(456);
    m.m[1] = 2;
    m.m[2] = 3;
    m.v.emplace<int64_t>(777);
    m.os = "foo";

    Message m1{};
    protopug::parse_from_string(m1, protopug::serialize_as_string(m));
    
    return 0;
}
```
