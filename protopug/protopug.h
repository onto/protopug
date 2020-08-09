#pragma once

#include <string>
#include <iostream>
#include <utility>
#include <cstddef>
#include <cstring>
#include <vector>
#include <optional>
#include <variant>
#include <map>
#include <cmath>

namespace protopug
{
    namespace detail
    {
        template<uint32_t Tag, class T, T, uint32_t Flags>
        struct field_impl
        {
        };

        template<uint32_t Tag, class T, class U, U T::*PtrToM, uint32_t Flags>
        struct field_impl<Tag, U T::*, PtrToM, Flags>
        {
            using type = U;
            constexpr static const uint32_t tag = Tag;
            constexpr static const uint32_t flags = Flags;

            static decltype(auto) get(const T &value)
            {
                return value.*PtrToM;
            }

            static decltype(auto) get(T &value)
            {
                return value.*PtrToM;
            }
        };

        template<uint32_t Tag, size_t Index, class T, T, uint32_t Flags>
        struct oneof_field_impl
        {
        };

        template<uint32_t Tag, size_t Index, class T, class U, U T::*PtrToM, uint32_t Flags>
        struct oneof_field_impl<Tag, Index, U T::*, PtrToM, Flags>
        {
            using type =  U;
            constexpr static const uint32_t tag = Tag;
            constexpr static const size_t index = Index;
            constexpr static const uint32_t flags = Flags;

            static decltype(auto) get(const T &value)
            {
                return value.*PtrToM;
            }

            static decltype(auto) get(T &value)
            {
                return value.*PtrToM;
            }
        };

        template<uint32_t Tag, class T, T, uint32_t KeyFlags, uint32_t ValueFlags>
        struct map_field_impl
        {
        };

        template<uint32_t Tag, class T, class U, U T::*PtrToM, uint32_t KeyFlags, uint32_t ValueFlags>
        struct map_field_impl<Tag, U T::*, PtrToM, KeyFlags, ValueFlags>
        {
            using type =  U;
            constexpr static const uint32_t tag = Tag;
            constexpr static const uint32_t key_flags = KeyFlags;
            constexpr static const uint32_t value_flags = ValueFlags;

            static decltype(auto) get(const T &value)
            {
                return value.*PtrToM;
            }

            static decltype(auto) get(T &value)
            {
                return value.*PtrToM;
            }
        };
    }

    enum class WireType : uint32_t
    {
        Varint = 0,
        Fixed64 = 1,
        LengthDelimeted = 2,
        StartGroup = 3,
        EndGroup = 4,
        Fixed32 = 5,
    };

    bool is_valid_wire_type(WireType wire_type)
    {
        switch (wire_type)
        {
        case WireType::Varint:
        case WireType::Fixed64:
        case WireType::LengthDelimeted:
        case WireType::StartGroup:
        case WireType::EndGroup:
        case WireType::Fixed32:
            return true;
        default:
            return false;
        }
    }

    namespace flags
    {
        enum flags
        {
            no = 0,
            s = 1,
            f = 2
        };
    }

    template<uint32_t flags = flags::no>
    struct flags_t {};

    template<class T>
    struct descriptor
    {
        static_assert(sizeof(T) == 0, "You need to implement descriptor for your own types");

        using type = void;
    };

    template<class... Fields>
    struct message
    {
    };

    template<uint32_t Tag, auto Arg, uint32_t Flags = flags::no>
    using field = detail::field_impl<Tag, decltype(Arg), Arg, Flags>;

    template<uint32_t Tag, size_t Index, auto Arg, uint32_t Flags = flags::no>
    using oneof_field = detail::oneof_field_impl<Tag, Index, decltype(Arg), Arg, Flags>;

    template<uint32_t Tag, auto Arg, uint32_t KeyFlags = flags::no, uint32_t ValueFlags = flags::no>
    using map_field = detail::map_field_impl<Tag, decltype(Arg), Arg, KeyFlags, ValueFlags>;

    template<class T, class Enable = void>
    struct serializer;

    struct writer
    {
        virtual void write(const void *bytes, size_t size) = 0;
    };

    struct reader
    {
        virtual size_t read(void *bytes, size_t size) = 0;
    };

    template<class T>
    void write_message(const T &value, writer &out);

    template<class T>
    bool read_message(T &value, reader &in);

    namespace detail
    {
        template<class T, class V, class F, class W, class Enable = void>
        struct has_serialize_packed : public std::false_type
        {};

        template<class T, class V, class F, class W>
        struct has_serialize_packed<T, V, F, W, std::void_t<decltype(std::declval<T>().serialize_packed(std::declval<V>(), std::declval<F>(), std::declval<W &>()))>> :
                public std::true_type
        {};

        template<class T, class V, class F, class W>
        constexpr bool has_serialize_packed_v = has_serialize_packed<T, V, F, W>::value;

        template<class T, class V, class F, class R, class Enable = void>
        struct has_parse_packed : public std::false_type
        {};

        template<class T, class V, class F, class R>
        struct has_parse_packed<T, V, F, R, std::void_t<decltype(std::declval<T>().parse_packed(std::declval<V &>(), std::declval<F>(), std::declval<R &>()))>> :
                public std::true_type
        {};

        template<class T, class V, class F, class R>
        constexpr bool has_parse_packed_v = has_parse_packed<T, V, F, R>::value;

        uint32_t make_tag_wire_type(uint32_t tag, WireType wire_type)
        {
            return (tag << 3) | static_cast<uint32_t>(wire_type);
        }

        void read_tag_wire_type(uint32_t tag_key, uint32_t &tag, WireType &wire_type)
        {
            wire_type = static_cast<WireType>(tag_key & 0b0111);
            tag = tag_key >> 3;
        }

        uint32_t make_zigzag_value(int32_t value)
        {
            uint32_t s_value = static_cast<uint32_t>(value);
            return (s_value << 1) ^ (s_value >> 31);
        }

        uint64_t make_zigzag_value(int64_t value)
        {
            uint64_t s_value = static_cast<uint64_t>(value);
            return (s_value << 1) ^ (s_value >> 63);
        }

        int32_t read_zigzag_value(uint32_t value)
        {
            return static_cast<int32_t>((value >> 1) ^ (~(value & 1) + 1));
        }

        int64_t read_zigzag_value(uint64_t value)
        {
            return static_cast<int64_t>((value >> 1) ^ (~(value & 1) + 1));
        }

        template<class To, class From>
        To bit_cast(From from)
        {
            static_assert(sizeof(To) == sizeof(From), "");
            static_assert(std::is_trivially_copyable_v<To>, "");
            static_assert(std::is_trivially_copyable_v<From>, "");

            To to;
            std::memcpy(&to, &from, sizeof(from));
            return to;
        }

        struct writer_size_collector : public writer
        {
            void write(const void */*bytes*/, size_t size) override
            {
                byte_size += size;
            }
            size_t byte_size = 0;
        };

        struct limited_reader : public reader
        {
            limited_reader(reader &parent, size_t size_limit)
                : _parent(parent)
                , _size_limit(size_limit)
            {}

            size_t read(void *bytes, size_t size)
            {
                auto size_to_read = std::min(size, _size_limit);
                auto read_size = _parent.read(bytes, size_to_read);
                _size_limit -= read_size;
                return read_size;
            }

        private:
            reader &_parent;
            size_t _size_limit;
        };

        void write_byte(uint8_t value, writer &out)
        {
            out.write(&value, 1);
        }

        bool read_byte(uint8_t &value, reader &in)
        {
            return in.read(&value, 1) == 1;
        }

        void write_varint(uint32_t value, writer &out)
        {
            do
            {
                uint8_t x = value & 0b0111'1111;
                value >>= 7;
                if (value)
                {
                    write_byte(x | 0b1000'0000, out);
                }
                else
                {
                    write_byte(x, out);
                    break;
                }
            }
            while (true);
        }

        void write_varint(uint64_t value, writer &out)
        {
            do
            {
                uint8_t x = value & 0b0111'1111;
                value >>= 7;
                if (value)
                {
                    write_byte(x | 0b1000'0000, out);
                }
                else
                {
                    write_byte(x, out);
                    break;
                }
            }
            while (true);
        }

        bool read_varint(uint32_t &value, reader &in)
        {
            value = 0;
            for (size_t c = 0; c < 5 /*(32 / 7) + 1*/; ++c)
            {
                uint8_t x;
                if (!read_byte(x, in))
                    return false;

                value |= static_cast<uint32_t>(x & 0b0111'1111) << 7 * c;
                if (!(x & 0b1000'0000))
                {
                    return true;
                }
            }

            return false;
        }

        bool read_varint(uint64_t &value, reader &in)
        {
            value &= 0;
            for (size_t c = 0; c < 10 /*(64 / 7) + 1*/; ++c)
            {
                uint8_t x;
                if (!read_byte(x, in))
                    return false;

                value |= static_cast<uint64_t>(x & 0b0111'1111) << 7 * c;
                if (!(x & 0b1000'0000))
                {
                    return true;
                }
            }

            return false;
        }

        void write_fixed(uint32_t value, writer &out)
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            out.write(&value, sizeof(value));
#else
            static_assert(false, "Not a little-endian");
#endif
        }

        void write_fixed(uint64_t value, writer &out)
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            out.write(&value, sizeof(value));
#else
            static_assert(false, "Not a little-endian");
#endif
        }

        void write_fixed(double value, writer &out)
        {
            write_fixed(bit_cast<uint64_t>(value), out);
        }

        void write_fixed(float value, writer &out)
        {
            write_fixed(bit_cast<uint32_t>(value), out);
        }

        void write_varint(int32_t value, writer &out)
        {
            write_varint(bit_cast<uint32_t>(value), out);
        }

        void write_varint(int64_t value, writer &out)
        {
            write_varint(bit_cast<uint64_t>(value), out);
        }

        void write_signed_varint(int32_t value, writer &out)
        {
            write_varint(make_zigzag_value(value), out);
        }

        void write_signed_varint(int64_t value, writer &out)
        {
            write_varint(make_zigzag_value(value), out);
        }

        void write_signed_fixed(int32_t value, writer &out)
        {
            write_fixed(make_zigzag_value(value), out);
        }

        void write_signed_fixed(int64_t value, writer &out)
        {
            write_fixed(make_zigzag_value(value), out);
        }

        void write_tag_wire_type(uint32_t tag, WireType wire_type, writer &out)
        {
            write_varint(make_tag_wire_type(tag, wire_type), out);
        }

        bool read_fixed(uint32_t &value, reader &in)
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return in.read(&value, sizeof(value)) == sizeof(value);
#else
            static_assert(false, "Not a little-endian");
#endif
        }

        bool read_fixed(uint64_t &value, reader &in)
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return in.read(&value, sizeof(value)) == sizeof(value);
#else
            static_assert(false, "Not a little-endian");
#endif
        }

        bool read_fixed(double &value, reader &in)
        {
            uint64_t intermediate_value;
            if (read_fixed(intermediate_value, in))
            {
                value = bit_cast<double>(intermediate_value);
                return true;
            }
            return false;
        }

        bool read_fixed(float &value, reader &in)
        {
            uint32_t intermediate_value;
            if (read_fixed(intermediate_value, in))
            {
                value = bit_cast<float>(intermediate_value);
                return true;
            }
            return false;
        }

        bool read_varint(int32_t &value, reader &in)
        {
            uint32_t intermediate_value;
            if (read_varint(intermediate_value, in))
            {
                value = bit_cast<int32_t>(intermediate_value);
                return true;
            }
            return false;
        }

        bool read_varint(int64_t &value, reader &in)
        {
            uint64_t intermediate_value;
            if (read_varint(intermediate_value, in))
            {
                value = bit_cast<int64_t>(intermediate_value);
                return true;
            }
            return false;
        }

        bool read_signed_varint(int32_t &value, reader &in)
        {
            uint32_t intermediate_value;
            if (read_varint(intermediate_value, in))
            {
                value = read_zigzag_value(intermediate_value);
                return true;
            }
            return false;
        }

        bool read_signed_varint(int64_t &value, reader &in)
        {
            uint64_t intermediate_value;
            if (read_varint(intermediate_value, in))
            {
                value = read_zigzag_value(intermediate_value);
                return true;
            }
            return false;
        }

        bool read_signed_fixed(int32_t &value, reader &in)
        {
            uint32_t intermediate_value;
            if (read_fixed(intermediate_value, in))
            {
                value = read_zigzag_value(intermediate_value);
                return true;
            }
            return false;
        }

        bool read_signed_fixed(int64_t &value, reader &in)
        {
            uint64_t intermediate_value;
            if (read_fixed(intermediate_value, in))
            {
                value = read_zigzag_value(intermediate_value);
                return true;
            }
            return false;
        }

        template<class T, uint32_t Tag, size_t Index, auto Arg, uint32_t Flags>
        void write_field(const T &value, oneof_field<Tag, Index, Arg, Flags>, writer &out)
        {
            using OneOf = oneof_field<Tag, Index, Arg, Flags>;
            serializer<typename OneOf::type>::template serialize_oneof<OneOf::index>(OneOf::tag, OneOf::get(value), flags_t<OneOf::flags>(), out);
        }

        template<class T, uint32_t Tag, auto Arg, uint32_t KeyFlags, uint32_t ValueFlags>
        void write_field(const T &value, map_field<Tag, Arg, KeyFlags, ValueFlags>, writer &out)
        {
            using Field = map_field<Tag, Arg, KeyFlags, ValueFlags>;
            serializer<typename Field::type>::serialize_map(Field::tag, Field::get(value), flags_t<Field::key_flags>(), flags_t<Field::value_flags>(),
                    out);
        }

        template<class T, uint32_t Tag, auto Arg, uint32_t Flags>
        void write_field(const T &value, field<Tag, Arg, Flags>, writer &out)
        {
            using Field = field<Tag, Arg, Flags>;
            serializer<typename Field::type>::serialize(Field::tag, Field::get(value), flags_t<Field::flags>(), out);
        }

        template<class T, class... Field>
        void write_message(const T &value, message<Field...>, writer &out)
        {
            (write_field(value, Field(), out), ...);
        }

        template<uint32_t Flags, class ValueType, class It>
        void write_repeated(uint32_t Tag, It begin, It end, writer &out)
        {
            if (begin == end) return;

            if constexpr(detail::has_serialize_packed_v<serializer<ValueType>, ValueType, flags_t<Flags>, writer>)
            {
                write_varint(make_tag_wire_type(Tag, WireType::LengthDelimeted), out);

                writer_size_collector size_collector;
                for (auto it = begin; it != end; ++it)
                {
                    serializer<ValueType>::serialize_packed(*it, flags_t<Flags> {}, size_collector);
                }

                write_varint(size_collector.byte_size, out);

                for (auto it = begin; it != end; ++it)
                {
                    serializer<ValueType>::serialize_packed(*it, flags_t<Flags> {}, out);
                }
            }
            else
            {
                for (auto it = begin; it != end; ++it)
                {
                    serializer<ValueType>::serialize(Tag, *it, flags_t<Flags>(), out);
                }
            }
        }

        template<uint32_t KeyFlags, uint32_t ValueFlags, class Key, class Value>
        void write_map_key_value(const std::pair<const Key, Value> &value, writer &out)
        {
            serializer<Key>::serialize(1, value.first, flags_t<KeyFlags> {}, out, true);
            serializer<Value>::serialize(2, value.second, flags_t<ValueFlags> {}, out, true);
        }

        template<uint32_t KeyFlags, uint32_t ValueFlags, class T>
        void write_map(uint32_t Tag, const T &value, writer &out)
        {
            auto begin = std::begin(value);
            auto end = std::end(value);

            for (auto it = begin; it != end; ++it)
            {
                write_tag_wire_type(Tag, WireType::LengthDelimeted, out);

                writer_size_collector size_collector;
                write_map_key_value<KeyFlags, ValueFlags>(*it, size_collector);

                write_varint(size_collector.byte_size, out);
                write_map_key_value<KeyFlags, ValueFlags>(*it, out);
            }
        }

        template<uint32_t KeyFlags, uint32_t ValueFlags, class Key, class Value>
        bool read_map_key_value(std::pair<Key, Value> &value, reader &in)
        {
            using PairAsMessage = message<field<1, &std::pair<Key, Value>::first, KeyFlags>, field<2, &std::pair<Key, Value>::second, ValueFlags>>;
            return read_message(value, PairAsMessage(), in);
        }

        template<uint32_t KeyFlags, uint32_t ValueFlags, class T>
        bool read_map(WireType wire_type, T &value, reader &in)
        {
            if (wire_type != WireType::LengthDelimeted) return false;

            size_t size;
            if (read_varint(size, in))
            {
                limited_reader limited_in(in, size);

                do
                {
                    std::pair<typename T::key_type, typename T::mapped_type> item;
                    if (!read_map_key_value<KeyFlags, ValueFlags>(item, limited_in))
                        break;

                    value.insert(std::move(item));
                }
                while (true);

                return true;
            }
            return false;
        }

        template<uint32_t Flags, class ValueType, class OutputIt>
        bool read_repeated(WireType wire_type, OutputIt output_it, reader &in)
        {
            if constexpr(detail::has_parse_packed_v<serializer<ValueType>, ValueType, flags_t<Flags>, reader>)
            {
                if (wire_type != WireType::LengthDelimeted) return false;

                size_t size;
                if (read_varint(size, in))
                {
                    limited_reader limited_in(in, size);

                    do
                    {
                        ValueType value;
                        if (serializer<ValueType>::parse_packed(value, flags_t<Flags>(), limited_in))
                        {
                            output_it = value;
                            ++output_it;
                        }
                        else
                        {
                            break;
                        }
                    }
                    while (true);

                    return true;
                }

                return false;
            }
            else
            {
                ValueType value;
                if (serializer<ValueType>::parse(wire_type, value, flags_t<Flags>(), in))
                {
                    output_it = value;
                    ++output_it;
                    return true;
                }

                return false;
            }
        }

        template<class T, uint32_t Tag, size_t Index, auto Arg, uint32_t Flags>
        void read_field(T &value, uint32_t tag, WireType wire_type, oneof_field<Tag, Index, Arg, Flags>, reader &in)
        {
            if (Tag != tag) return;

            using OneOf = oneof_field<Tag, Index, Arg, Flags>;
            serializer<typename OneOf::type>::template parse_oneof<OneOf::index>(wire_type, OneOf::get(value), flags_t<OneOf::flags>(), in);
        }

        template<class T, uint32_t Tag, auto Arg, uint32_t KeyFlags, uint32_t ValueFlags>
        void read_field(T &value, uint32_t tag, WireType wire_type, map_field<Tag, Arg, KeyFlags, ValueFlags>, reader &in)
        {
            if (Tag != tag) return;

            using Field = map_field<Tag, Arg, KeyFlags, ValueFlags>;
            serializer<typename Field::type>::parse_map(wire_type, Field::get(value), flags_t<Field::key_flags>(), flags_t<Field::value_flags>(), in);
        }

        template<class T, uint32_t Tag, auto Arg, uint32_t Flags>
        void read_field(T &value, uint32_t tag, WireType wire_type, field<Tag, Arg, Flags>, reader &in)
        {
            if (Tag != tag) return;

            using Field = field<Tag, Arg, Flags>;
            serializer<typename Field::type>::parse(wire_type, Field::get(value), flags_t<Field::flags>(), in);
        }

        template<class T, class... Field>
        bool read_message(T &value, message<Field...>, reader &in)
        {
            uint32_t tag_key;
            while (read_varint(tag_key, in))
            {
                uint32_t tag;
                WireType wire_type;

                read_tag_wire_type(tag_key, tag, wire_type);

                (read_field(value, tag, wire_type, Field(), in), ...);
            }

            return true;
        }
    }

    template<class T>
    void write_message(const T &value, writer &out)
    {
        using Message = typename descriptor<T>::type;
        detail::write_message(value, Message(), out);
    }

    template<class T>
    bool read_message(T &value, reader &in)
    {
        using Message = typename descriptor<T>::type;
        return detail::read_message(value, Message(), in);
    }

    template<class T, class Enable>
    struct serializer
    {
        // Commion serializer threat type as message
        static void serialize(uint32_t tag, const T &value, flags_t<>, writer &out, bool force = false)
        {
            detail::writer_size_collector size_collector;
            write_message(value, size_collector);

            if (!force && size_collector.byte_size == 0) return;

            detail::write_tag_wire_type(tag, WireType::LengthDelimeted, out);
            detail::write_varint(size_collector.byte_size, out);
            write_message(value, out);
        }

        static bool parse(WireType wire_type, T &value, flags_t<>, reader &in)
        {
            if (wire_type != WireType::LengthDelimeted) return false;

            size_t size;
            if (detail::read_varint(size, in))
            {
                detail::limited_reader limited_in(in, size);
                return read_message(value, limited_in);
            }

            return false;
        }
    };

    template<>
    struct serializer<int32_t>
    {
        static void serialize(uint32_t tag, int32_t value, flags_t<>, writer &out, bool force = false)
        {
            if (!force && value == INT32_C(0)) return;

            detail::write_tag_wire_type(tag, WireType::Varint, out);
            detail::write_varint(value, out);
        }

        static void serialize(uint32_t tag, int32_t value, flags_t<flags::s>, writer &out, bool force = false)
        {
            if (!force && value == INT32_C(0)) return;

            detail::write_tag_wire_type(tag, WireType::Varint, out);
            detail::write_signed_varint(value, out);
        }

        static void serialize(uint32_t tag, int32_t value, flags_t < flags::s | flags::f >,  writer &out, bool force = false)
        {
            if (!force && value == INT32_C(0)) return;

            detail::write_tag_wire_type(tag, WireType::Fixed32, out);
            detail::write_signed_fixed(value, out);
        }

        static void serialize_packed(int32_t value, flags_t<>, writer &out)
        {
            detail::write_varint(value, out);
        }

        static void serialize_packed(int32_t value, flags_t<flags::s>, writer &out)
        {
            detail::write_signed_varint(value, out);
        }

        static void serialize_packed(int32_t value, flags_t < flags::s | flags::f >,  writer &out)
        {
            detail::write_signed_fixed(value, out);
        }

        static bool parse(WireType wire_type, int32_t &value, flags_t<>, reader &in)
        {
            if (wire_type != WireType::Varint) return false;
            return detail::read_varint(value, in);
        }

        static bool parse(WireType wire_type, int32_t &value, flags_t<flags::s>, reader &in)
        {
            if (wire_type != WireType::Varint) return false;
            return detail::read_signed_varint(value, in);
        }

        static bool parse(WireType wire_type, int32_t &value, flags_t < flags::s | flags::f >, reader &in)
        {
            if (wire_type != WireType::Fixed32) return false;
            return detail::read_signed_fixed(value, in);
        }

        static bool parse_packed(int32_t &value, flags_t<>, reader &in)
        {
            return detail::read_varint(value, in);
        }

        static bool parse_packed(int32_t &value, flags_t<flags::s>, reader &in)
        {
            return detail::read_signed_varint(value, in);
        }

        static bool parse_packed(int32_t &value, flags_t < flags::s | flags::f >, reader &in)
        {
            return detail::read_signed_fixed(value, in);
        }
    };

    template<>
    struct serializer<uint32_t>
    {
        static void serialize(uint32_t tag, uint32_t value, flags_t<>, writer &out, bool force = false)
        {
            if (!force && value == UINT32_C(0)) return;

            detail::write_tag_wire_type(tag, WireType::Varint, out);
            detail::write_varint(value, out);
        }

        static void serialize(uint32_t tag, uint32_t value, flags_t<flags::f>, writer &out, bool force = false)
        {
            if (!force && value == UINT32_C(0)) return;

            detail::write_tag_wire_type(tag, WireType::Fixed32, out);
            detail::write_fixed(value, out);
        }

        static void serialize_packed(uint32_t value, flags_t<>, writer &out)
        {
            detail::write_varint(value, out);
        }

        static void serialize_packed(uint32_t value, flags_t<flags::f>, writer &out)
        {
            detail::write_fixed(value, out);
        }

        static bool parse(WireType wire_type, uint32_t &value, flags_t<>, reader &in)
        {
            if (wire_type != WireType::Varint) return false;
            return detail::read_varint(value, in);
        }

        static bool parse(WireType wire_type, uint32_t &value, flags_t<flags::f>, reader &in)
        {
            if (wire_type != WireType::Fixed32) return false;
            return detail::read_fixed(value, in);
        }

        static bool parse_packed(uint32_t &value, flags_t<>, reader &in)
        {
            return detail::read_varint(value, in);
        }

        static bool parse_packed(uint32_t &value, flags_t<flags::f>, reader &in)
        {
            return detail::read_fixed(value, in);
        }
    };

    template<>
    struct serializer<int64_t>
    {
        static void serialize(uint32_t tag, int64_t value, flags_t<>, writer &out, bool force = false)
        {
            if (!force && value == INT64_C(0)) return;

            detail::write_tag_wire_type(tag, WireType::Varint, out);
            detail::write_varint(value, out);
        }

        static void serialize(uint32_t tag, int64_t value, flags_t<flags::s>, writer &out, bool force = false)
        {
            if (!force && value == INT64_C(0)) return;

            detail::write_tag_wire_type(tag, WireType::Varint, out);
            detail::write_signed_varint(value, out);
        }

        static void serialize(uint32_t tag, int64_t value, flags_t < flags::s | flags::f >, writer &out, bool force = false)
        {
            if (!force && value == INT64_C(0)) return;

            detail::write_tag_wire_type(tag, WireType::Fixed64, out);
            detail::write_signed_fixed(value, out);
        }

        static void serialize_packed(int64_t value, flags_t<>, writer &out)
        {
            detail::write_varint(value, out);
        }

        static void serialize_packed(int64_t value, flags_t<flags::s>, writer &out)
        {
            detail::write_signed_varint(value, out);
        }

        static void serialize_packed(int64_t value, flags_t < flags::s | flags::f >, writer &out)
        {
            detail::write_signed_fixed(value, out);
        }

        static bool parse(WireType wire_type, int64_t &value, flags_t<>, reader &in)
        {
            if (wire_type != WireType::Varint) return false;
            return detail::read_varint(value, in);
        }

        static bool parse(WireType wire_type, int64_t &value, flags_t<flags::s>, reader &in)
        {
            if (wire_type != WireType::Varint) return false;
            return detail::read_signed_varint(value, in);
        }

        static bool parse(WireType wire_type, int64_t &value, flags_t < flags::s | flags::f >, reader &in)
        {
            if (wire_type != WireType::Fixed64) return false;
            return detail::read_signed_fixed(value, in);
        }

        static bool parse_packed(int64_t &value, flags_t<>, reader &in)
        {
            return detail::read_varint(value, in);
        }

        static bool parse_packed(int64_t &value, flags_t<flags::s>, reader &in)
        {
            return detail::read_signed_varint(value, in);
        }

        static bool parse_packed(int64_t &value, flags_t < flags::s | flags::f >, reader &in)
        {
            return detail::read_signed_fixed(value, in);
        }
    };

    template<>
    struct serializer<uint64_t>
    {
        static void serialize(uint32_t tag, uint64_t value, flags_t<>, writer &out, bool force = false)
        {
            if (!force && value == UINT64_C(0)) return;

            detail::write_tag_wire_type(tag, WireType::Varint, out);
            detail::write_varint(value, out);
        }

        static void serialize(uint32_t tag, uint64_t value, flags_t<flags::f>, writer &out, bool force = false)
        {
            if (!force && value == UINT64_C(0)) return;

            detail::write_tag_wire_type(tag, WireType::Fixed64, out);
            detail::write_fixed(value, out);
        }

        static void serialize_packed(uint64_t value, flags_t<>, writer &out)
        {
            detail::write_varint(value, out);
        }

        static void serialize_packed(uint64_t value, flags_t<flags::f>, writer &out)
        {
            detail::write_fixed(value, out);
        }

        static bool parse(WireType wire_type, uint64_t &value, flags_t<>, reader &in)
        {
            if (wire_type != WireType::Varint) return false;
            return detail::read_varint(value, in);
        }

        static bool parse(WireType wire_type, uint64_t &value, flags_t<flags::f>, reader &in)
        {
            if (wire_type != WireType::Fixed64) return false;
            return detail::read_fixed(value, in);
        }

        static bool parse_packed(uint64_t &value, flags_t<>, reader &in)
        {
            return detail::read_varint(value, in);
        }

        static bool parse_packed(uint64_t &value, flags_t<flags::f>, reader &in)
        {
            return detail::read_fixed(value, in);
        }
    };

    template<>
    struct serializer<double>
    {
        static void serialize(uint32_t tag, double value, flags_t<>, writer &out, bool force = false)
        {
            if (!force && std::fpclassify(value) == FP_ZERO) return;

            detail::write_tag_wire_type(tag, WireType::Fixed64, out);
            detail::write_fixed(value, out);
        }

        static void serialize_packed(double value, flags_t<>, writer &out)
        {
            detail::write_fixed(value, out);
        }

        static bool parse(WireType wire_type, double &value, flags_t<>, reader &in)
        {
            if (wire_type != WireType::Fixed64) return false;
            return detail::read_fixed(value, in);
        }

        static bool parse_packed(double &value, flags_t<>, reader &in)
        {
            return detail::read_fixed(value, in);
        }
    };

    template<>
    struct serializer<float>
    {
        static void serialize(uint32_t tag, float value, flags_t<>, writer &out, bool force = false)
        {
            if (!force && std::fpclassify(value) == FP_ZERO) return;

            detail::write_tag_wire_type(tag, WireType::Fixed32, out);
            detail::write_fixed(value, out);
        }

        static void serialize_packed(float value, flags_t<>, writer &out)
        {
            detail::write_fixed(value, out);
        }

        static bool parse(WireType wire_type, float &value, flags_t<>, reader &in)
        {
            if (wire_type != WireType::Fixed32) return false;
            return detail::read_fixed(value, in);
        }

        static bool parse_packed(float &value, flags_t<>, reader &in)
        {
            return detail::read_fixed(value, in);
        }
    };

    template<>
    struct serializer<bool>
    {
        static void serialize(uint32_t tag, bool value, flags_t<>, writer &out, bool force = false)
        {
            serializer<uint32_t>::serialize(tag, value ? 1 : 0, flags_t(), out, force);
        }

        static void serialize_packed(bool value, flags_t<>, writer &out)
        {
            serializer<uint32_t>::serialize_packed(value ? 1 : 0, flags_t(), out);
        }

        static bool parse(WireType wire_type, bool &value, flags_t<>, reader &in)
        {
            uint32_t intermedaite_value;
            if (serializer<uint32_t>::parse(wire_type, intermedaite_value, flags_t<>(), in))
            {
                value = static_cast<bool>(intermedaite_value);
                return true;
            }
            return false;
        }

        static bool parse_packed(bool &value, flags_t<>, reader &in)
        {
            uint32_t intermedaite_value;
            if (serializer<uint32_t>::parse_packed(intermedaite_value, flags_t<>(), in))
            {
                value = static_cast<bool>(intermedaite_value);
                return true;
            }
            return false;
        }
    };

    template<class T>
    struct serializer<T, std::enable_if_t<std::is_enum_v<T>>>
    {
        using U = std::underlying_type_t<T>;

        static void serialize(uint32_t tag, T value, flags_t<>, writer &out, bool force = false)
        {
            serializer<U>::serialize(tag, static_cast<U>(value), flags_t<>(), out, force);
        }

        static void serialize_packed(T value, flags_t<>, writer &out)
        {
            serializer<U>::serialize_packed(static_cast<U>(value), flags_t<>(), out);
        }

        static bool parse(WireType wire_type, T &value, flags_t<>, reader &in)
        {
            U intermedaite_value;
            if (serializer<U>::parse(wire_type, intermedaite_value, flags_t<>(), in))
            {
                value = static_cast<T>(intermedaite_value);
                return true;
            }
            return false;
        }

        static bool parse_packed(T &value, flags_t<>, reader &in)
        {
            U intermedaite_value;
            if (serializer<U>::parse_packed(intermedaite_value, flags_t<>(), in))
            {
                value = static_cast<T>(intermedaite_value);
                return true;
            }
            return false;
        }
    };

    template<>
    struct serializer<std::string>
    {
        static void serialize(uint32_t tag, const std::string &value, flags_t<>, writer &out, bool force = false)
        {
            if (!force && value.empty()) return;

            detail::write_tag_wire_type(tag, WireType::LengthDelimeted, out);
            detail::write_varint(value.size(), out);
            out.write(value.data(), value.size());
        }

        static bool parse(WireType wire_type, std::string &value, flags_t<>, reader &in)
        {
            if (wire_type != WireType::LengthDelimeted) return false;

            size_t size;
            if (detail::read_varint(size, in))
            {
                value.resize(size);
                if (in.read(value.data(), size) == size)
                {
                    return true;
                }
            }

            return false;
        }
    };

    template<class T>
    struct serializer<std::vector<T>>
    {
        template<uint32_t Flags>
        static void serialize(uint32_t tag, const std::vector<T> &value, flags_t<Flags>, writer &out)
        {
            detail::write_repeated<Flags, T>(tag, value.begin(), value.end(), out);
        }

        template<uint32_t Flags>
        static bool parse(WireType wire_type, std::vector<T> &value, flags_t<Flags>, reader &in)
        {
            return detail::read_repeated<Flags, T>(wire_type, std::back_inserter(value), in);
        }
    };

    template<class T>
    struct serializer<std::optional<T>>
    {
        template<uint32_t Flags>
        static void serialize(uint32_t tag, const std::optional<T> &value, flags_t<Flags>, writer &out)
        {
            if (!value.has_value()) return;

            serializer<T>::serialize(tag, *value, flags_t<Flags>(), out);
        }

        template<uint32_t Flags>
        static bool parse(WireType wire_type, std::optional<T> &value, flags_t<Flags>, reader &in)
        {
            return serializer<T>::parse(wire_type, value.emplace(), flags_t<Flags>(), in);
        }
    };

    template<class... T>
    struct serializer<std::variant<T...>>
    {
        template<size_t Index, uint32_t Flags>
        static void serialize_oneof(uint32_t tag, const std::variant<T...> &value, flags_t<Flags>, writer &out)
        {
            if (value.index() != Index) return;

            serializer<std::variant_alternative_t<Index, std::variant<T...>>>::serialize(tag, std::get<Index>(value), flags_t<Flags>(), out);
        }

        template<size_t Index, uint32_t Flags>
        static bool parse_oneof(WireType wire_type, std::variant<T...> &value, flags_t<Flags>, reader &in)
        {
            return serializer<std::variant_alternative_t<Index, std::variant<T...>>>::parse(wire_type, value.template emplace<Index>(),
                    flags_t<Flags>(), in);
        }
    };

    template<class Key, class Value>
    struct serializer<std::map<Key, Value>>
    {
        template<uint32_t KeyFlags, uint32_t ValueFlags>
        static void serialize_map(uint32_t tag, const std::map<Key, Value> &value, flags_t<KeyFlags>, flags_t<ValueFlags>, writer &out)
        {
            detail::write_map<KeyFlags, ValueFlags>(tag, value, out);
        }

        template<uint32_t KeyFlags, uint32_t ValueFlags>
        static bool parse_map(WireType wire_type, std::map<Key, Value> &value, flags_t<KeyFlags>, flags_t<ValueFlags>, reader &in)
        {
            return detail::read_map<KeyFlags, ValueFlags>(wire_type, value, in);
        }
    };

    struct string_writer : public writer
    {
        string_writer(std::string &out)
            : _out(out)
        {}

        void write(const void *bytes, size_t size) override
        {
            _out.append(reinterpret_cast<const char *>(bytes), size);
        }
    private:
        std::string &_out;
    };

    struct string_reader : public reader
    {
        string_reader(const std::string &in)
            : _in(in)
            , _pos(0)
        {}

        size_t read(void *bytes, size_t size) override
        {
            size_t read_size = std::min(size, _in.size() - _pos);
            memcpy(bytes, _in.data() + _pos, read_size);
            _pos += read_size;
            return read_size;
        }

    private:
        const std::string &_in;
        size_t _pos;
    };

    template <class T>
    void serialize_to_string(const T &value, std::string &out)
    {
        string_writer string_out(out);
        write_message(value, string_out);
    }

    template <class T>
    std::string serialize_as_string(const T &value)
    {
        std::string out;
        serialize_to_string(value, out);
        return out;
    }

    template <class T>
    bool parse_from_string(T &value, const std::string &in)
    {
        string_reader string_in(in);
        return read_message(value, string_in);
    }
}

