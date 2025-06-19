
// ��� ��������� �����
template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type
get_random(T min, T max) {
    thread_local std::random_device rd; // ������������� ���� ��� �� �����
    thread_local std::mt19937 gen(rd()); // ��������� ��� ������� ������
    thread_local std::uniform_real_distribution<T> dis(min, max); // ������������� ��� ������� ������
    return dis(gen);
}

// ��� ������������� �����
template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
get_random(T min, T max) {
    thread_local std::random_device rd; // ������������� ���� ��� �� �����
    thread_local std::mt19937 gen(rd()); // ��������� ��� ������� ������
    thread_local std::uniform_int_distribution<T> dis(min, max); // ������������� ��� ������� ������
    return dis(gen);
}

// ������� ��������� Xorshift128+ (������������� ��� ����������� CPU)
inline uint64_t xorshift128(uint64_t state[2]) {
    uint64_t s1 = state[0];
    uint64_t s0 = state[1];
    state[0] = s0;
    s1 ^= s1 << 23;
    state[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    return state[1] + s0;
}

// �������� ������������ ��������� (LGC) - ��� ������ CPU
inline uint32_t lcg(uint32_t& state) {
    state = state * 214013 + 2531011;
    return (state >> 16) & 0x7FFF;
}

// ��������� �� ������ ������� ����������
inline uint64_t time_seed() {
    uint32_t lo, hi;
    asm volatile (
        "rdtsc" : "=a"(lo), "=d"(hi)
    );
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

// ������� ��������������� � �������� [min, max]
template <typename T>
inline T fast_scale(uint64_t rand_val, T min, T max) {
    const double scale = static_cast<double>(rand_val) / static_cast<double>(UINT64_MAX);
    return static_cast<T>(min + scale * (max - min));
}
//����������� ��� ������ �����
template <typename T>
inline T fast_rand(T min, T max, uint64_t seed = 0) {
    static uint64_t state[2] = {
        seed ? seed : time_seed(),
        seed ? seed ^ 0xDEADBEEF : time_seed() ^ 0xCAFEBABE
    };

    // ����� ���������� � ����������� �� ����
    uint64_t rand_val;
    if constexpr (sizeof(T) <= 4) {
        rand_val = lcg(reinterpret_cast<uint32_t&>(state[0]));
    } else {
        rand_val = xorshift128(state);
    }

    return fast_scale(rand_val, min, max);
}
//����������������� �����������
template <typename T>
inline void vector_rand(T min, T max, T* output, size_t count) {
    constexpr size_t block_size = (64 / sizeof(T)) > 4 ? (64 / sizeof(T)) : 4;
    alignas(64) uint64_t state_block[2 * block_size];

    // ������������� ����� ���������
    const uint64_t seed = time_seed();
    for(size_t i = 0; i < 2 * block_size; ++i) {
        state_block[i] = seed + i * 0x9E3779B97F4A7C15;
    }

    size_t generated = 0;
    while(generated < count) {
        // ��������� ����� ��������
        for(size_t i = 0; i < block_size && generated < count; ++i, ++generated) {
            uint64_t rand_val;
            if constexpr (sizeof(T) <= 4) {
                rand_val = lcg(reinterpret_cast<uint32_t&>(state_block[i % (2 * block_size)]));
            } else {
                rand_val = xorshift128(&state_block[2 * i]);
            }

            output[generated] = fast_scale(rand_val, min, max);
        }

        // "�������������" ��������� ��� ���������� �����
        for(size_t i = 0; i < 2 * block_size; ++i) {
            state_block[i] = xorshift128(state_block) ^ state_block[i];
        }
    }
}
