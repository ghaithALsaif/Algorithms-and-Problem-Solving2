// ============================================================================
// game01.cpp
// A complete, self-contained Dark Fantasy Voxel Strategy Game Architecture.
// Backend: Simulation Engine (Terrain, Fluids, Economy, Politics, Construction, Darkness).
// Frontend: Raylib 3D Renderer with Interactive UI.
// ============================================================================

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <cmath>
#include <string>
#include <string_view>
#include <array>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <random>
#include <optional>
#include <cassert>
#include <cstdlib>

#include "raylib.h"

// ============================================================================
// 1. FORWARD DECLARATIONS
// ============================================================================
namespace voxel {
    class World;
    class Chunk;
    class PerlinNoise;
    struct Voxel;
    struct RaycastHit;
    class WorldGenerator;
    class FluidSimulator;
}

namespace economy {
    class Settlement;
    class NPC;
    class Market;
    class ProfessionSystem;
    enum class Commodity : uint8_t;
}

namespace politics {
    class Kingdom;
    class DiplomacyMatrix;
    class KingdomAI;
    class ReputationManager;
}

namespace construction {
    class ConstructionManager;
    struct ConstructionZone;
}

namespace darkness {
    class WorldClock;
    class MonsterManager;
    class MagicCrafting;
    struct Monster;
}

// ============================================================================
// 2. GLM MOCK & STD HASH (Standalone Math Types)
// ============================================================================
namespace glm {
    struct vec3 {
        float x = 0, y = 0, z = 0;
        vec3() = default;
        vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
        
        float& operator[](int i) { return (i == 0) ? x : (i == 1) ? y : z; }
        const float& operator[](int i) const { return (i == 0) ? x : (i == 1) ? y : z; }
    };
    struct ivec3 {
        int x = 0, y = 0, z = 0;
        ivec3() = default;
        ivec3(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
        
        int& operator[](int i) { return (i == 0) ? x : (i == 1) ? y : z; }
        const int& operator[](int i) const { return (i == 0) ? x : (i == 1) ? y : z; }
    };

    inline ivec3 floor(vec3 v) { return ivec3((int)std::floor(v.x), (int)std::floor(v.y), (int)std::floor(v.z)); }
    inline ivec3 operator+(const ivec3& a, const ivec3& b) { return ivec3(a.x + b.x, a.y + b.y, a.z + b.z); }
    inline bool operator==(const ivec3& a, const ivec3& b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
    inline vec3 operator/(vec3 v, float f) { return vec3(v.x / f, v.y / f, v.z / f); }
    inline vec3 operator-(const vec3& a, const vec3& b) { return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
}

// ============================================================================
// 3. CORE VOXEL SYSTEM (namespace voxel)
// ============================================================================
namespace voxel {

enum class VoxelType : uint8_t {
    Air = 0, Stone = 1, Dirt = 2, Grass = 3, Sand = 4, Sandstone = 5,
    Water = 6, Lava = 7, Iron_Ore = 8, Gold_Ore = 9, Bedrock = 10,
    CorruptedStone = 11, Wood = 12, Count
};

struct Voxel {
    uint8_t data_ = 0;
    constexpr Voxel() noexcept = default;
    constexpr Voxel(VoxelType t, uint8_t level = 1) noexcept
        : data_(static_cast<uint8_t>((static_cast<uint8_t>(t) & 0x0F) | ((level & 0x07) << 4))) {}

    [[nodiscard]] constexpr VoxelType type() const noexcept { return static_cast<VoxelType>(data_ & 0x0F); }
    [[nodiscard]] constexpr uint8_t level() const noexcept { return (data_ >> 4) & 0x07; }
    [[nodiscard]] constexpr bool isFertile() const noexcept { return (data_ >> 7) & 0x01; }
    constexpr void setType(VoxelType t) noexcept { data_ = (data_ & 0xF0) | (static_cast<uint8_t>(t) & 0x0F); }
    constexpr void setLevel(uint8_t l) noexcept { data_ = (data_ & 0x8F) | ((l & 0x07) << 4); }
    constexpr void setFertile(bool f) noexcept { data_ = (data_ & 0x7F) | (f ? 0x80 : 0x00); }
    constexpr bool operator==(const Voxel& o) const noexcept { return data_ == o.data_; }
};
static_assert(sizeof(Voxel) == 1, "Voxel must be exactly 1 byte");

struct VoxelInfo { const char* name; bool solid; bool liquid; uint8_t baseDurability; };
inline constexpr std::array<VoxelInfo, static_cast<size_t>(VoxelType::Count)> kVoxelInfo = {{
    {"Air", false, false, 0}, {"Stone", true, false, 5}, {"Dirt", true, false, 3},
    {"Grass", true, false, 3}, {"Sand", true, false, 2}, {"Sandstone", true, false, 4},
    {"Water", false, true, 0}, {"Lava", false, true, 0}, {"Iron_Ore", true, false, 6},
    {"Gold_Ore", true, false, 7}, {"Bedrock", true, false, 7}, {"CorruptedStone", true, false, 5},
    {"Wood", true, false, 4}
}};
[[nodiscard]] inline constexpr bool isSolid(VoxelType t) noexcept { return kVoxelInfo[static_cast<size_t>(t)].solid; }
[[nodiscard]] inline constexpr bool isLiquid(VoxelType t) noexcept { return kVoxelInfo[static_cast<size_t>(t)].liquid; }
[[nodiscard]] inline constexpr bool isAir(VoxelType t) noexcept { return t == VoxelType::Air; }

inline constexpr int kChunkSizeX = 16, kChunkSizeY = 256, kChunkSizeZ = 16;
inline constexpr int kChunkVolume = kChunkSizeX * kChunkSizeY * kChunkSizeZ;

class Chunk {
public:
    Chunk(int32_t cx, int32_t cz) noexcept : cx_(cx), cz_(cz) {
        voxels_.fill(Voxel(VoxelType::Air, 0));
    }
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    [[nodiscard]] Voxel getVoxel(int lx, int ly, int lz) const noexcept {
        assert(lx >= 0 && lx < kChunkSizeX && ly >= 0 && ly < kChunkSizeY && lz >= 0 && lz < kChunkSizeZ);
        return voxels_[index(lx, ly, lz)];
    }
    void setVoxel(int lx, int ly, int lz, Voxel v) noexcept {
        assert(lx >= 0 && lx < kChunkSizeX && ly >= 0 && ly < kChunkSizeY && lz >= 0 && lz < kChunkSizeZ);
        voxels_[index(lx, ly, lz)] = v;
        dirty_ = true;
    }
    [[nodiscard]] int32_t chunkX() const noexcept { return cx_; }
    [[nodiscard]] int32_t chunkZ() const noexcept { return cz_; }
    [[nodiscard]] int32_t worldOriginX() const noexcept { return cx_ * kChunkSizeX; }
    [[nodiscard]] int32_t worldOriginZ() const noexcept { return cz_ * kChunkSizeZ; }
    [[nodiscard]] bool isDirty() const noexcept { return dirty_; }
    void markDirty(bool d = true) noexcept { dirty_ = d; }
    [[nodiscard]] const Voxel* data() const noexcept { return voxels_.data(); }
    [[nodiscard]] Voxel* data() noexcept { return voxels_.data(); }
    static constexpr int index(int lx, int ly, int lz) noexcept { return lx + lz * kChunkSizeX + ly * kChunkSizeX * kChunkSizeZ; }
private:
    std::array<Voxel, kChunkVolume> voxels_;
    int32_t cx_, cz_;
    bool dirty_ = true;
};

class PerlinNoise {
public:
    explicit PerlinNoise(uint32_t seed) noexcept {
        int p[256];
        for (int i = 0; i < 256; ++i) p[i] = i;
        std::mt19937 rng(seed);
        std::shuffle(p, p + 256, rng);
        for (int i = 0; i < 256; ++i) { perm_[i] = p[i]; perm_[i + 256] = p[i]; }
    }
    [[nodiscard]] float noise2D(float x, float y) const noexcept { return noise3D(x, y, 0.0f); }
    [[nodiscard]] float noise3D(float x, float y, float z) const noexcept {
        int X = static_cast<int>(std::floor(x)) & 255, Y = static_cast<int>(std::floor(y)) & 255, Z = static_cast<int>(std::floor(z)) & 255;
        x -= std::floor(x); y -= std::floor(y); z -= std::floor(z);
        float u = fade(x), v = fade(y), w = fade(z);
        int A = perm_[X] + Y, AA = perm_[A] + Z, AB = perm_[A + 1] + Z;
        int B = perm_[X + 1] + Y, BA = perm_[B] + Z, BB = perm_[B + 1] + Z;
        return lerp(lerp(lerp(grad(perm_[AA], x, y, z), grad(perm_[BA], x - 1, y, z), u),
                         lerp(grad(perm_[AB], x, y - 1, z), grad(perm_[BB], x - 1, y - 1, z), u), v),
                    lerp(lerp(grad(perm_[AA + 1], x, y, z - 1), grad(perm_[BA + 1], x - 1, y, z - 1), u),
                         lerp(grad(perm_[AB + 1], x, y - 1, z - 1), grad(perm_[BB + 1], x - 1, y - 1, z - 1), u), v), w);
    }
    [[nodiscard]] float fbm2D(float x, float y, int octaves = 4, float persistence = 0.5f, float lacunarity = 2.0f) const noexcept {
        float total = 0.0f, amplitude = 1.0f, frequency = 1.0f, maxAmp = 0.0f;
        for (int i = 0; i < octaves; ++i) { total += noise2D(x * frequency, y * frequency) * amplitude; maxAmp += amplitude; amplitude *= persistence; frequency *= lacunarity; }
        return total / maxAmp;
    }
private:
    int perm_[512];
    static float fade(float t) noexcept { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
    static float lerp(float a, float b, float t) noexcept { return a + t * (b - a); }
    static float grad(int hash, float x, float y, float z) noexcept {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};

enum class BiomeType : uint8_t { Forest, Desert, DarkMountainous, Ocean, Count };
struct ClimatePoint { float temperature; float moisture; };
struct BiomeContext {
    int worldX, worldZ, surfaceHeight, seaLevel;
    const PerlinNoise &heightNoise, &moistureNoise, &temperatureNoise, &oreNoise;
};

class IBiome {
public:
    virtual ~IBiome() = default;
    [[nodiscard]] virtual BiomeType type() const noexcept = 0;
    virtual void generateColumn(const BiomeContext& ctx, Voxel* column) const = 0;
};

namespace {
void fillStoneBase(Voxel* column, int height, int seaLevel) noexcept {
    for (int y = 0; y < kChunkSizeY; ++y) {
        if (y == 0) column[y] = Voxel(VoxelType::Bedrock, 7);
        else if (y < height) column[y] = Voxel(VoxelType::Stone, 5);
        else if (y <= seaLevel) column[y] = Voxel(VoxelType::Water, 7);
        else column[y] = Voxel(VoxelType::Air, 0);
    }
}
void scatterOres(Voxel* column, int height, const PerlinNoise& oreNoise, int worldX, int worldZ, float richness) noexcept {
    for (int y = 1; y < height; ++y) {
        if (column[y].type() != VoxelType::Stone) continue;
        float n = oreNoise.noise3D(worldX * 0.10f, y * 0.10f, worldZ * 0.10f) * 0.5f + 0.5f;
        if (y < 32 && n > 0.92f - richness * 0.05f) column[y].setType(VoxelType::Gold_Ore);
        else if (n > 0.85f - richness * 0.05f) column[y].setType(VoxelType::Iron_Ore);
    }
}
}

class ForestBiome : public IBiome {
public:
    BiomeType type() const noexcept override { return BiomeType::Forest; }
    void generateColumn(const BiomeContext& ctx, Voxel* column) const override {
        int h = ctx.surfaceHeight; fillStoneBase(column, h, ctx.seaLevel);
        if (h > ctx.seaLevel) {
            column[h] = Voxel(VoxelType::Grass, 3);
            for (int y = std::max(1, h - 3); y < h; ++y) column[y] = Voxel(VoxelType::Dirt, 3);
        } else { for (int y = std::max(1, h - 2); y <= h; ++y) column[y] = Voxel(VoxelType::Sand, 2); }
        scatterOres(column, h, ctx.oreNoise, ctx.worldX, ctx.worldZ, 0.0f);
    }
};

class DesertBiome : public IBiome {
public:
    BiomeType type() const noexcept override { return BiomeType::Desert; }
    void generateColumn(const BiomeContext& ctx, Voxel* column) const override {
        int h = ctx.surfaceHeight; fillStoneBase(column, h, ctx.seaLevel);
        float duneN = ctx.moistureNoise.noise2D(ctx.worldX * 0.05f, ctx.worldZ * 0.05f) * 0.5f + 0.5f;
        int sandDepth = 4 + static_cast<int>(duneN * 4.0f);
        for (int y = std::max(1, h - sandDepth); y <= h; ++y) column[y] = (y == h) ? Voxel(VoxelType::Sand, 2) : Voxel(VoxelType::Sandstone, 4);
        scatterOres(column, h, ctx.oreNoise, ctx.worldX, ctx.worldZ, 0.0f);
    }
};

class DarkMountainousBiome : public IBiome {
public:
    BiomeType type() const noexcept override { return BiomeType::DarkMountainous; }
    void generateColumn(const BiomeContext& ctx, Voxel* column) const override {
        int h = ctx.surfaceHeight; fillStoneBase(column, h, ctx.seaLevel);
        if (h > ctx.seaLevel) {
            if (h > ctx.seaLevel + 30) column[h] = Voxel(VoxelType::Stone, 6);
            else { column[h] = Voxel(VoxelType::Dirt, 4); for (int y = std::max(1, h - 2); y < h; ++y) column[y] = Voxel(VoxelType::Dirt, 4); }
        }
        scatterOres(column, h, ctx.oreNoise, ctx.worldX, ctx.worldZ, 1.0f);
    }
};

[[nodiscard]] inline BiomeType selectBiome(float temperature, float moisture, float altitude) noexcept {
    if (altitude > 0.70f) return BiomeType::DarkMountainous;
    if (temperature < 0.30f && moisture < 0.35f) return BiomeType::DarkMountainous;
    if (temperature > 0.65f && moisture < 0.35f) return BiomeType::Desert;
    return BiomeType::Forest;
}
[[nodiscard]] inline std::shared_ptr<IBiome> getBiome(BiomeType type) {
    switch (type) {
        case BiomeType::Forest: return std::make_shared<ForestBiome>();
        case BiomeType::Desert: return std::make_shared<DesertBiome>();
        case BiomeType::DarkMountainous: return std::make_shared<DarkMountainousBiome>();
        default: return std::make_shared<ForestBiome>();
    }
}

struct WorldGenConfig { uint32_t seed = 1337; int seaLevel = 64; int baseHeight = 64; int amplitude = 32; };
class WorldGenerator {
public:
    explicit WorldGenerator(WorldGenConfig config = {}) noexcept
        : config_(config), heightNoise_(config.seed), temperatureNoise_(config.seed * 7u + 13u),
          moistureNoise_(config.seed * 31u + 101u), oreNoise_(config.seed * 53u + 211u), caveNoise_(config.seed * 97u + 307u) {}

    void generateChunk(Chunk& chunk) const {
        int32_t originX = chunk.worldOriginX(), originZ = chunk.worldOriginZ();
        std::array<Voxel, kChunkSizeY> column{};
        for (int lz = 0; lz < kChunkSizeZ; ++lz) {
            for (int lx = 0; lx < kChunkSizeX; ++lx) {
                int wx = originX + lx, wz = originZ + lz;
                ClimatePoint climate = computeClimate(wx, wz);
                int estH = computeSurfaceHeight(wx, wz, BiomeType::Forest);
                float altitude = static_cast<float>(estH) / static_cast<float>(kChunkSizeY);
                BiomeType biome = selectBiome(climate.temperature, climate.moisture, altitude);
                int surfaceHeight = computeSurfaceHeight(wx, wz, biome);
                BiomeContext ctx{wx, wz, surfaceHeight, config_.seaLevel, heightNoise_, moistureNoise_, temperatureNoise_, oreNoise_};
                auto biomePtr = getBiome(biome);
                biomePtr->generateColumn(ctx, column.data());
                for (int y = 0; y < kChunkSizeY; ++y) chunk.setVoxel(lx, y, lz, column[y]);
            }
        }
        chunk.markDirty(true);
    }
    [[nodiscard]] int computeSurfaceHeight(int worldX, int worldZ, BiomeType biome) const {
        float continent = heightNoise_.fbm2D(worldX * 0.0015f, worldZ * 0.0015f, 4, 0.5f, 2.0f) * 0.5f + 0.5f;
        float hills = heightNoise_.fbm2D(worldX * 0.010f + 500.0f, worldZ * 0.010f + 500.0f, 4, 0.5f, 2.0f) * 0.5f + 0.5f;
        int h = config_.baseHeight;
        switch (biome) {
            case BiomeType::Forest: h += static_cast<int>(continent * 20.0f + hills * 12.0f - 6.0f); break;
            case BiomeType::Desert: h += static_cast<int>(continent * 8.0f + hills * 4.0f); break;
            case BiomeType::DarkMountainous: {
                float ridged = 1.0f - std::abs(heightNoise_.fbm2D(worldX * 0.008f, worldZ * 0.008f, 5, 0.55f, 2.1f));
                ridged = std::pow(ridged,2.0f);
                h += static_cast<int>(continent * 30.0f + ridged * 80.0f);
                break;
            }
            default: h += static_cast<int>(continent * 10.0f + hills * 8.0f); break;
        }
        return std::clamp(h, 1, kChunkSizeY - 2);
    }
    [[nodiscard]] ClimatePoint computeClimate(int worldX, int worldZ) const {
        float t = temperatureNoise_.fbm2D(worldX * 0.0015f, worldZ * 0.0015f, 3, 0.5f, 2.0f) * 0.5f + 0.5f;
        float m = moistureNoise_.fbm2D(worldX * 0.0018f + 1000.0f, worldZ * 0.0018f + 1000.0f, 3, 0.5f, 2.0f) * 0.5f + 0.5f;
        return {t, m};
    }
    [[nodiscard]] BiomeType computeBiome(int worldX, int worldZ) const {
        ClimatePoint climate = computeClimate(worldX, worldZ);
        int estH = computeSurfaceHeight(worldX, worldZ, BiomeType::Forest);
        float altitude = static_cast<float>(estH) / static_cast<float>(kChunkSizeY);
        return selectBiome(climate.temperature, climate.moisture, altitude);
    }
    [[nodiscard]] const WorldGenConfig& config() const noexcept { return config_; }
private:
    WorldGenConfig config_;
    PerlinNoise heightNoise_, temperatureNoise_, moistureNoise_, oreNoise_, caveNoise_;
};

struct ChunkKey { int32_t x, z; bool operator==(const ChunkKey& o) const noexcept { return x == o.x && z == o.z; } };
struct ChunkKeyHash { std::size_t operator()(const ChunkKey& k) const noexcept { return std::hash<uint64_t>{}((static_cast<uint64_t>(k.x) << 32) | static_cast<uint32_t>(k.z)); } };
struct RaycastHit { glm::ivec3 blockPos, prevBlockPos; };

class World {
public:
    explicit World(uint32_t seed) : generator_(WorldGenConfig{seed, 64, 64, 32}) {}

    [[nodiscard]] Chunk* getChunk(int32_t cx, int32_t cz) {
        ChunkKey key{cx, cz};
        { std::lock_guard<std::mutex> lock(chunksMutex_); auto it = chunks_.find(key); if (it != chunks_.end()) return it->second.get(); }
        auto chunk = std::make_unique<Chunk>(cx, cz);
        generator_.generateChunk(*chunk);
        Chunk* raw = chunk.get();
        { std::lock_guard<std::mutex> lock(chunksMutex_); auto it = chunks_.find(key); if (it != chunks_.end()) return it->second.get(); chunks_[key] = std::move(chunk); }
        return raw;
    }
    [[nodiscard]] const Chunk* getChunk(int32_t cx, int32_t cz) const { std::lock_guard<std::mutex> lock(chunksMutex_); auto it = chunks_.find(ChunkKey{cx, cz}); return (it != chunks_.end()) ? it->second.get() : nullptr; }

    [[nodiscard]] Voxel getVoxelUnsafe(int wx, int wy, int wz) const noexcept {
        if (wy < 0 || wy >= kChunkSizeY) return Voxel{};
        const Chunk* c = getChunkUnsafe(worldToChunk(wx, kChunkSizeX), worldToChunk(wz, kChunkSizeZ));
        return c ? c->getVoxel(worldToLocal(wx, kChunkSizeX), wy, worldToLocal(wz, kChunkSizeZ)) : Voxel{};
    }
    void setVoxelUnsafe(int wx, int wy, int wz, Voxel v) noexcept {
        if (wy < 0 || wy >= kChunkSizeY) return;
        if (Chunk* c = getChunkUnsafe(worldToChunk(wx, kChunkSizeX), worldToChunk(wz, kChunkSizeZ))) c->setVoxel(worldToLocal(wx, kChunkSizeX), wy, worldToLocal(wz, kChunkSizeZ), v);
    }
    [[nodiscard]] Chunk* getChunkUnsafe(int32_t cx, int32_t cz) noexcept { auto it = chunks_.find(ChunkKey{cx, cz}); return (it != chunks_.end()) ? it->second.get() : nullptr; }
    [[nodiscard]] const Chunk* getChunkUnsafe(int32_t cx, int32_t cz) const noexcept { auto it = chunks_.find(ChunkKey{cx, cz}); return (it != chunks_.end()) ? it->second.get() : nullptr; }

    [[nodiscard]] Voxel getVoxel(int wx, int wy, int wz) const { std::lock_guard<std::mutex> lock(chunksMutex_); return getVoxelUnsafe(wx, wy, wz); }
    void setVoxel(int wx, int wy, int wz, Voxel v) { std::lock_guard<std::mutex> lock(chunksMutex_); setVoxelUnsafe(wx, wy, wz, v); }

    void ensureChunksAround(int32_t px, int32_t pz, int radius) { 
        int32_t pcx = worldToChunk(px, kChunkSizeX); 
        int32_t pz_c = worldToChunk(pz, kChunkSizeZ); 
        for (int dz = -radius; dz <= radius; ++dz) {
            for (int dx = -radius; dx <= radius; ++dx) {
                (void)getChunk(pcx + dx, pz_c + dz); 
            }
        }
    }
    void unloadDistantChunks(int32_t px, int32_t pz, int radius) {
        int32_t pcx = worldToChunk(px, kChunkSizeX), pcz = worldToChunk(pz, kChunkSizeZ);
        std::lock_guard<std::mutex> lock(chunksMutex_);
        for (auto it = chunks_.begin(); it != chunks_.end(); ) { if (std::abs(it->first.x - pcx) > radius || std::abs(it->first.z - pcz) > radius) it = chunks_.erase(it); else ++it; }
    }
    [[nodiscard]] std::size_t loadedChunkCount() const noexcept { std::lock_guard<std::mutex> lock(chunksMutex_); return chunks_.size(); }

    [[nodiscard]] const WorldGenerator& generator() const noexcept { return generator_; }
    [[nodiscard]] std::mutex& getChunksMutex() noexcept { return chunksMutex_; }

    [[nodiscard]] std::optional<RaycastHit> raycastVoxel(const glm::vec3& origin, const glm::vec3& dir, float maxDist) const {
        glm::ivec3 current = glm::floor(origin);
        glm::vec3 tMax, tDelta; glm::ivec3 step;
        for (int i = 0; i < 3; ++i) {
            if (dir[i] > 0) { step[i] = 1; tDelta[i] = 1.0f / dir[i]; tMax[i] = (static_cast<float>(current[i] + 1) - origin[i]) / dir[i]; }
            else if (dir[i] < 0) { step[i] = -1; tDelta[i] = -1.0f / dir[i]; tMax[i] = (static_cast<float>(current[i]) - origin[i]) / dir[i]; }
            else { step[i] = 0; tDelta[i] = std::numeric_limits<float>::infinity(); tMax[i] = std::numeric_limits<float>::infinity(); }
        }
        std::lock_guard<std::mutex> lock(chunksMutex_);
        float dist = 0.0f; glm::ivec3 prevBlock = current;
        while (dist <= maxDist) {
            if (isSolid(getVoxelUnsafe(current.x, current.y, current.z).type())) return RaycastHit{current, prevBlock};
            prevBlock = current;
            if (tMax.x < tMax.y) { if (tMax.x < tMax.z) { current.x += step.x; dist = tMax.x; tMax.x += tDelta.x; } else { current.z += step.z; dist = tMax.z; tMax.z += tDelta.z; } }
            else { if (tMax.y < tMax.z) { current.y += step.y; dist = tMax.y; tMax.y += tDelta.y; } else { current.z += step.z; dist = tMax.z; tMax.z += tDelta.z; } }
        }
        return std::nullopt;
    }
    void breakVoxel(const glm::vec3& origin, const glm::vec3& dir, float maxDist) { if (auto hit = raycastVoxel(origin, dir, maxDist)) setVoxel(hit->blockPos.x, hit->blockPos.y, hit->blockPos.z, Voxel(VoxelType::Air, 0)); }
    void placeVoxel(const glm::vec3& origin, const glm::vec3& dir, float maxDist, VoxelType type) { if (auto hit = raycastVoxel(origin, dir, maxDist)) { Voxel target = getVoxel(hit->prevBlockPos.x, hit->prevBlockPos.y, hit->prevBlockPos.z); if (isAir(target.type()) || isLiquid(target.type())) setVoxel(hit->prevBlockPos.x, hit->prevBlockPos.y, hit->prevBlockPos.z, Voxel(type, 7)); } }

private:
    WorldGenerator generator_;
    std::unordered_map<ChunkKey, std::unique_ptr<Chunk>, ChunkKeyHash> chunks_;
    mutable std::mutex chunksMutex_;

    inline int32_t worldToChunk(int32_t w, int cs) const noexcept { return (w >= 0) ? (w / cs) : ((w - cs + 1) / cs); }
    inline int32_t worldToLocal(int32_t w, int cs) const noexcept { int32_t v = w % cs; return (v < 0) ? v + cs : v; }
};

struct IVec3Hash { std::size_t operator()(const glm::ivec3& v) const noexcept { return std::hash<int>{}(v.x) ^ (std::hash<int>{}(v.y) << 1) ^ (std::hash<int>{}(v.z) << 2); } };
class FluidSimulator {
public:
    void addActiveWater(int x, int y, int z) { activeWaterBlocks_.insert({x, y, z}); }
    void tick(World& world) {
        std::lock_guard<std::mutex> lock(world.getChunksMutex());
        std::unordered_set<glm::ivec3, IVec3Hash> nextActive;
        for (const auto& pos : activeWaterBlocks_) processBlock(world, pos.x, pos.y, pos.z, nextActive);
        activeWaterBlocks_ = std::move(nextActive);
    }
private:
    std::unordered_set<glm::ivec3, IVec3Hash> activeWaterBlocks_;
    void processBlock(World& world, int x, int y, int z, std::unordered_set<glm::ivec3, IVec3Hash>& nextActive) {
        Voxel current = world.getVoxelUnsafe(x, y, z);
        if (current.type() != VoxelType::Water) return;
        uint8_t level = current.level();
        if (level == 0) return;
        irrigateNeighbors(world, x, y, z);
        Voxel below = world.getVoxelUnsafe(x, y - 1, z);
        if (isAir(below.type()) || (below.type() == VoxelType::Water && below.level() < 7)) {
            uint8_t transfer = std::min(static_cast<uint8_t>(7 - below.level()), level);
            world.setVoxelUnsafe(x, y - 1, z, Voxel(VoxelType::Water, below.level() + transfer));
            nextActive.insert({x, y - 1, z});
            if (level - transfer == 0) { world.setVoxelUnsafe(x, y, z, Voxel(VoxelType::Air, 0)); return; }
            else { current.setLevel(level - transfer); world.setVoxelUnsafe(x, y, z, current); nextActive.insert({x, y, z}); return; }
        }
        if (level > 1) {
            const int dx[] = {1, -1, 0, 0}, dz[] = {0, 0, 1, -1};
            bool spread = false;
            for (int i = 0; i < 4; ++i) {
                Voxel neighbor = world.getVoxelUnsafe(x + dx[i], y, z + dz[i]);
                if (isAir(neighbor.type()) || (neighbor.type() == VoxelType::Water && neighbor.level() < level - 1)) {
                    uint8_t nLevel = (neighbor.type() == VoxelType::Water) ? neighbor.level() : 0;
                    uint8_t transfer = std::min(static_cast<uint8_t>(level - 1 - nLevel), level);
                    if (transfer > 0) { world.setVoxelUnsafe(x + dx[i], y, z + dz[i], Voxel(VoxelType::Water, nLevel + transfer)); nextActive.insert({x + dx[i], y, z + dz[i]}); spread = true; }
                }
            }
            if (spread) { current.setLevel(level - 1); world.setVoxelUnsafe(x, y, z, current); nextActive.insert({x, y, z}); }
        }
    }
    void irrigateNeighbors(World& world, int x, int y, int z) {
        const int dx[] = {1, -1, 0, 0, 0, 0}, dy[] = {0, 0, 0, 0, 1, -1}, dz[] = {0, 0, 1, -1, 0, 0};
        for (int i = 0; i < 6; ++i) {
            Voxel neighbor = world.getVoxelUnsafe(x + dx[i], y + dy[i], z + dz[i]);
            if ((neighbor.type() == VoxelType::Dirt || neighbor.type() == VoxelType::Grass) && !neighbor.isFertile()) {
                neighbor.setFertile(true); world.setVoxelUnsafe(x + dx[i], y + dy[i], z + dz[i], neighbor);
            }
        }
    }
};

} // namespace voxel


// ============================================================================
// 4. ECONOMY BASE (namespace economy)
// ============================================================================
namespace economy {

enum class Commodity : uint8_t {
    Food = 0, Wood = 1, Stone = 2, IronOre = 3, GoldOre = 4, Weapons = 5, Tools = 6, MagicPotions = 7,
    MonsterEssence = 8, Potion_FireResist = 9, Potion_Invisibility = 10, Potion_BerserkRage = 11, Count
};
inline constexpr size_t kCommodityCount = static_cast<size_t>(Commodity::Count);
struct CommodityInfo { const char* name; int basePrice; bool isRawMaterial; bool isCrafted; };
inline constexpr std::array<CommodityInfo, kCommodityCount> kCommodityInfo = {{
    {"Food", 5, false, false}, {"Wood", 10, true, false}, {"Stone", 3, true, false},
    {"IronOre", 20, true, false}, {"GoldOre", 50, true, false}, {"Weapons", 100, false, true},
    {"Tools", 60, false, true}, {"MagicPotions", 150, false, true}, {"MonsterEssence", 0, false, false},
    {"Potion_FireResist", 150, false, true}, {"Potion_Invisibility", 200, false, true}, {"Potion_BerserkRage", 250, false, true}
}};
[[nodiscard]] inline constexpr const char* commodityName(Commodity c) noexcept { return kCommodityInfo[static_cast<size_t>(c)].name; }
[[nodiscard]] inline constexpr int commodityBasePrice(Commodity c) noexcept { return kCommodityInfo[static_cast<size_t>(c)].basePrice; }

enum class NPCProfession : uint8_t { None = 0, Farmer = 1, Miner = 2, Blacksmith = 3, Alchemist = 4, Count };
inline constexpr size_t kProfessionCount = static_cast<size_t>(NPCProfession::Count);
struct NPCProfessionInfo { const char* name; Commodity primaryOutput; float productionCooldown; };
inline constexpr std::array<NPCProfessionInfo, kProfessionCount> kProfessionInfo = {{
    {"None", Commodity::Food, 0.0f}, {"Farmer", Commodity::Food, 5.0f}, {"Miner", Commodity::Stone, 3.0f},
    {"Blacksmith", Commodity::Weapons, 8.0f}, {"Alchemist", Commodity::MagicPotions, 10.0f}
}};
[[nodiscard]] inline constexpr const char* professionName(NPCProfession p) noexcept { return kProfessionInfo[static_cast<size_t>(p)].name; }

class NPC {
public:
    NPC(uint32_t id, NPCProfession prof, glm::ivec3 position) noexcept : id_(id), profession_(prof), gold_(100), position_(position), target_(position), cooldown_(0.0f) {}
    [[nodiscard]] uint32_t id() const noexcept { return id_; }
    [[nodiscard]] NPCProfession profession() const noexcept { return profession_; }
    [[nodiscard]] int gold() const noexcept { return gold_; }
    [[nodiscard]] glm::ivec3 position() const noexcept { return position_; }
    [[nodiscard]] glm::ivec3 targetDestination() const noexcept { return target_; }
    [[nodiscard]] float productionCooldown() const noexcept { return cooldown_; }
    void setTargetDestination(glm::ivec3 t) noexcept { target_ = t; }
    void setPosition(glm::ivec3 p) noexcept { position_ = p; }
    [[nodiscard]] bool spendGold(int amount) noexcept { if (gold_ < amount) return false; gold_ -= amount; return true; }
    void earnGold(int amount) noexcept { gold_ += amount; }
    void tickCooldown(float dt) noexcept { cooldown_ -= dt; }
    void resetCooldown() noexcept { cooldown_ = kProfessionInfo[static_cast<size_t>(profession_)].productionCooldown; }
    void setCooldown(float c) noexcept { cooldown_ = c; }
private:
    uint32_t id_; NPCProfession profession_; int gold_; glm::ivec3 position_, target_; float cooldown_;
};

class Market {
public:
    Market() noexcept { supply_.fill(0); demand_.fill(kMinDemand); priceModifiers_.fill(1.0f); }
    [[nodiscard]] int getPrice(Commodity c) const { std::lock_guard<std::mutex> lock(mutex_); return computePriceLocked(c); }
    [[nodiscard]] int getSupply(Commodity c) const { std::lock_guard<std::mutex> lock(mutex_); return supply_[static_cast<size_t>(c)]; }
    [[nodiscard]] float getDemand(Commodity c) const { std::lock_guard<std::mutex> lock(mutex_); return demand_[static_cast<size_t>(c)]; }
    void setPriceModifier(Commodity c, float m) noexcept { std::lock_guard<std::mutex> lock(mutex_); priceModifiers_[static_cast<size_t>(c)] = std::clamp(m, 0.1f, 10.0f); }
    void addSupply(Commodity c, int q) { std::lock_guard<std::mutex> lock(mutex_); supply_[static_cast<size_t>(c)] += q; }
    void removeSupply(Commodity c, int q) { std::lock_guard<std::mutex> lock(mutex_); auto& s = supply_[static_cast<size_t>(c)]; s = std::max(0, s - q); }
    void registerDemand(Commodity c, float a) { std::lock_guard<std::mutex> lock(mutex_); demand_[static_cast<size_t>(c)] += a; }
    void reduceDemand(Commodity c, float a) { std::lock_guard<std::mutex> lock(mutex_); auto& d = demand_[static_cast<size_t>(c)]; d = std::max(kMinDemand, d - a); }

    [[nodiscard]] bool buyItem(NPC& buyer, Commodity item, int quantity) {
        int totalCost;
        { std::lock_guard<std::mutex> lock(mutex_); size_t i = static_cast<size_t>(item); if (supply_[i] < quantity) return false; totalCost = computePriceLocked(item) * quantity; supply_[i] -= quantity; demand_[i] += static_cast<float>(quantity); }
        if (!buyer.spendGold(totalCost)) { std::lock_guard<std::mutex> lock(mutex_); size_t i = static_cast<size_t>(item); supply_[i] += quantity; demand_[i] -= static_cast<float>(quantity); return false; }
        return true;
    }
    [[nodiscard]] bool sellItem(NPC& seller, Commodity item, int quantity) {
        int revenue;
        { std::lock_guard<std::mutex> lock(mutex_); size_t i = static_cast<size_t>(item); revenue = computePriceLocked(item) * quantity; supply_[i] += quantity; demand_[i] = std::max(kMinDemand, demand_[i] - static_cast<float>(quantity)); }
        seller.earnGold(revenue);
        return true;
    }
    void tick(float dt) { std::lock_guard<std::mutex> lock(mutex_); for (size_t i = 0; i < kCommodityCount; ++i) if (demand_[i] > kMinDemand) demand_[i] = std::max(kMinDemand, demand_[i] - kDemandDecayRate * dt); }
private:
    mutable std::mutex mutex_;
    std::array<int, kCommodityCount> supply_;
    std::array<float, kCommodityCount> demand_;
    std::array<float, kCommodityCount> priceModifiers_;
    static constexpr float kDemandDecayRate = 0.25f, kMinDemand = 1.0f;
    [[nodiscard]] int computePriceLocked(Commodity c) const noexcept {
        float d = std::max(demand_[static_cast<size_t>(c)], kMinDemand);
        float s = static_cast<float>(supply_[static_cast<size_t>(c)] + 1);
        return static_cast<int>(std::max(1.0f, commodityBasePrice(c) * (d / s) * priceModifiers_[static_cast<size_t>(c)]));
    }
};

class ProfessionSystem; // Forward decl for Settlement
class Settlement;       // Forward decl for ConstructionManager

} // namespace economy


// ============================================================================
// 5. POLITICS & AI (namespace politics)
// ============================================================================
namespace politics {

enum class DiplomaticState : uint8_t { Peace = 0, War = 1, Alliance = 2 };
class DiplomacyMatrix {
public:
    explicit DiplomacyMatrix(size_t kc) : states_(kc * kc, DiplomaticState::Peace), size_(kc) {}
    void expandForNewKingdom() { std::unique_lock<std::shared_mutex> lock(mutex_); ++size_; states_.resize(size_ * size_, DiplomaticState::Peace); }
    [[nodiscard]] DiplomaticState getState(uint32_t a, uint32_t b) const { if (a == b) return DiplomaticState::Alliance; std::shared_lock<std::shared_mutex> lock(mutex_); return states_[getIndex(a, b)]; }
    void setState(uint32_t a, uint32_t b, DiplomaticState s) { if (a == b) return; std::unique_lock<std::shared_mutex> lock(mutex_); states_[getIndex(a, b)] = s; }
private:
    mutable std::shared_mutex mutex_;
    std::vector<DiplomaticState> states_;
    size_t size_;
    [[nodiscard]] size_t getIndex(uint32_t a, uint32_t b) const noexcept { if (a > b) std::swap(a, b); return static_cast<size_t>(a) * size_ + b; }
};

class Kingdom {
public:
    Kingdom(uint32_t id, std::string name, uint32_t kingNpcId) : id_(id), name_(std::move(name)), kingNpcId_(kingNpcId) {}
    [[nodiscard]] uint32_t id() const noexcept { return id_; }
    [[nodiscard]] uint32_t kingId() const noexcept { return kingNpcId_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] int treasury() const noexcept { return treasury_; }
    [[nodiscard]] int militaryPower() const noexcept { return militaryPower_; }
    void adjustTreasury(int a) { treasury_ += a; }
    void adjustMilitaryPower(int a) { militaryPower_ = std::max(0, militaryPower_ + a); }
    void addSettlement(economy::Settlement* s) { settlements_.push_back(s); }
    void removeSettlement(economy::Settlement* s) { settlements_.erase(std::remove(settlements_.begin(), settlements_.end(), s), settlements_.end()); }
    [[nodiscard]] const std::vector<economy::Settlement*>& settlements() const noexcept { return settlements_; }
    [[nodiscard]] int getTotalCommoditySupply(economy::Commodity c) const;
    [[nodiscard]] int getAverageCommodityPrice(economy::Commodity c) const;
    [[nodiscard]] bool isTradeDisrupted() const noexcept { return tradeDisrupted_; }
    void setTradeDisrupted(bool d) noexcept { tradeDisrupted_ = d; }
private:
    friend class KingdomAI;
    uint32_t id_; std::string name_; uint32_t kingNpcId_;
    int treasury_ = 1000, militaryPower_ = 100;
    bool tradeDisrupted_ = false;
    std::vector<economy::Settlement*> settlements_;
};

class KingdomAI {
public:
    static void evaluateStrategicGoals(Kingdom& self, const std::vector<Kingdom*>& all, DiplomacyMatrix& d);
private:
    static void evaluateResourceWar(Kingdom& self, const std::vector<Kingdom*>& all, DiplomacyMatrix& d);
    static void evaluateAlliances(Kingdom& self, const std::vector<Kingdom*>& all, DiplomacyMatrix& d);
    static void launchSimulatedSiege(Kingdom& self, Kingdom& target, economy::Settlement& s, DiplomacyMatrix& d);
};

enum class ReputationTier : uint8_t { Exalted = 0, Friendly = 1, Neutral = 2, Hostile = 3, SwornEnemy = 4 };
class ReputationManager {
public:
    ReputationManager() = default;
    void modifyReputation(uint32_t pid, uint32_t kid, int amount);
    void modifyReputation(uint32_t pid, const Kingdom& k, int amount) { modifyReputation(pid, k.id(), amount); }
    [[nodiscard]] int getReputation(uint32_t pid, uint32_t kid) const;
    [[nodiscard]] ReputationTier getTier(uint32_t pid, uint32_t kid) const;
    [[nodiscard]] bool isHostile(uint32_t pid, uint32_t kid) const { return getTier(pid, kid) >= ReputationTier::Hostile; }
    void reportPlayerAction(uint32_t pid, Kingdom& target, std::string_view action, const std::vector<Kingdom*>& all, DiplomacyMatrix& d);
private:
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, int>> map_;
    mutable std::shared_mutex mutex_;
    static constexpr int kMax = 1000, kMin = -1000;
};

} // namespace politics


// ============================================================================
// 6. CONSTRUCTION (namespace construction)
// ============================================================================
namespace construction {

enum class ZoneType : uint8_t { Residential = 0, Industrial = 1, Agricultural = 2, Military = 3 };
struct TargetVoxel { glm::ivec3 localPos; voxel::VoxelType type; };
struct ConstructionZone {
    uint32_t id; ZoneType type; glm::ivec3 origin;
    std::vector<TargetVoxel> targetVoxels;
    int progress = 0, totalVoxels = 0;
    bool isComplete = false; uint32_t assignedBuilderId = 0; float buildTimer = 0.0f;
};

class ConstructionManager {
public:
    ConstructionManager() = default;
    void queueStructureBlueprint(ZoneType type, glm::ivec3 origin);
    void tickConstruction(economy::Settlement& settlement, voxel::World& world, float dt);
    [[nodiscard]] bool buyOutConstruction(uint32_t zoneId, politics::Kingdom& kingdom, voxel::World& world);
    [[nodiscard]] const std::vector<ConstructionZone>& activeZones() const noexcept { return zones_; }
private:
    std::vector<ConstructionZone> zones_;
    std::unordered_set<uint32_t> assignedNpcs_;
    void generateResidentialBlueprint(ConstructionZone& z);
    void generateMilitaryBlueprint(ConstructionZone& z);
    void onZoneComplete(economy::Settlement& s, voxel::World& w, const ConstructionZone& z);
    static constexpr int kMaxPlacementsPerTick = 16;
};

} // namespace construction


// ============================================================================
// 7. ECONOMY SETTLEMENT (namespace economy)
// ============================================================================
namespace economy {

class Settlement {
public:
    Settlement(uint32_t id, glm::ivec3 center, voxel::BiomeType biome) noexcept : id_(id), center_(center), biome_(biome) {}
    void addNPC(std::unique_ptr<NPC> npc) { npcs_.push_back(std::move(npc)); }
    void removeNPC(uint32_t npcId) { npcs_.erase(std::remove_if(npcs_.begin(), npcs_.end(), [npcId](const std::unique_ptr<NPC>& n) { return n->id() == npcId; }), npcs_.end()); }
    [[nodiscard]] std::size_t npcCount() const noexcept { return npcs_.size(); }
    [[nodiscard]] const std::vector<std::unique_ptr<NPC>>& npcs() const noexcept { return npcs_; }
    
    void updateEconomy(float dt, voxel::World& world);
    [[nodiscard]] bool buyItem(NPC& e, Commodity i, int q) { return market_.buyItem(e, i, q); }
    [[nodiscard]] bool sellItem(NPC& e, Commodity i, int q) { return market_.sellItem(e, i, q); }

    [[nodiscard]] uint32_t id() const noexcept { return id_; }
    [[nodiscard]] glm::ivec3 center() const noexcept { return center_; }
    [[nodiscard]] voxel::BiomeType biome() const noexcept { return biome_; }
    [[nodiscard]] Market& market() noexcept { return market_; }
    [[nodiscard]] const Market& market() const noexcept { return market_; }
    [[nodiscard]] construction::ConstructionManager& constructionManager() noexcept { return constructionManager_; }
    [[nodiscard]] int maxNPCs() const noexcept { return maxNPCs_; }
    void adjustMaxNPCs(int amount) noexcept { maxNPCs_ += amount; }
    [[nodiscard]] float agricultureBonus() const noexcept { return agricultureBonus_; }
    void adjustAgricultureBonus(float bonus) noexcept { agricultureBonus_ += bonus; }

private:
    uint32_t id_; glm::ivec3 center_; voxel::BiomeType biome_;
    Market market_;
    std::vector<std::unique_ptr<NPC>> npcs_;
    construction::ConstructionManager constructionManager_;
    int maxNPCs_ = 10; float agricultureBonus_ = 0.0f;
    float woodAccumulator_ = 0.0f, stoneAccumulator_ = 0.0f;
    void tickPassiveResources(float dt);
};

} // namespace economy


// ============================================================================
// 8. DARKNESS & MAGIC (namespace darkness)
// ============================================================================
namespace darkness {

enum class WorldState : uint8_t { Daytime = 0, EclipseNight = 1 };
class WorldClock {
public:
    WorldClock() noexcept : currentTime_(8.0f) {}
    void tick(float dt) noexcept { 
        std::lock_guard<std::mutex> lock(mutex_);
        currentTime_ += dt * (1.0f / 60.0f); 
        if (currentTime_ >= 24.0f) currentTime_ -= 24.0f; 
    }
    [[nodiscard]] float getTime() const noexcept { 
        std::lock_guard<std::mutex> lock(mutex_);
        return currentTime_; 
    }
    [[nodiscard]] WorldState getState() const noexcept { 
        std::lock_guard<std::mutex> lock(mutex_);
        return (currentTime_ >= 18.0f || currentTime_ < 6.0f) ? WorldState::EclipseNight : WorldState::Daytime; 
    }
    [[nodiscard]] bool isNight() const noexcept { return getState() == WorldState::EclipseNight; }
private:
    mutable std::mutex mutex_;
    float currentTime_;
};

class MagicCrafting {
public:
    [[nodiscard]] static bool craftPotion(economy::NPC& witch, economy::Commodity potionType, economy::Settlement& localSettlement, const voxel::World& world) {
        if (world.generator().computeBiome(witch.position().x, witch.position().z) != voxel::BiomeType::DarkMountainous) return false;
        Recipe r; if (!getRecipe(potionType, r)) return false;
        economy::Market& m = localSettlement.market();
        if (m.getSupply(r.in1) < r.q1 || m.getSupply(r.in2) < r.q2) return false;
        m.removeSupply(r.in1, r.q1); m.removeSupply(r.in2, r.q2); m.addSupply(potionType, 1);
        std::cout << "[Magic] Witch brewed " << economy::commodityName(potionType) << "!\n";
        return true;
    }
private:
    struct Recipe { economy::Commodity in1; int q1; economy::Commodity in2; int q2; };
    [[nodiscard]] static bool getRecipe(economy::Commodity t, Recipe& r) {
        switch (t) {
            case economy::Commodity::Potion_FireResist: r = {economy::Commodity::GoldOre, 1, economy::Commodity::MonsterEssence, 1}; return true;
            case economy::Commodity::Potion_Invisibility: r = {economy::Commodity::GoldOre, 2, economy::Commodity::MonsterEssence, 2}; return true;
            case economy::Commodity::Potion_BerserkRage: r = {economy::Commodity::GoldOre, 3, economy::Commodity::MonsterEssence, 3}; return true;
            default: return false;
        }
    }
};

enum class MonsterType : uint8_t { ApostleSpawn = 0, Wraith = 1, Troll = 2 };
struct Monster { uint32_t id; MonsterType type; glm::vec3 position; int health; int attackPower; uint32_t targetSettlementId; };

class MonsterManager {
public:
    MonsterManager() = default;
    void tickMonsters(float dt, const WorldClock& clock, voxel::World& world, std::vector<economy::Settlement*>& settlements, std::vector<politics::Kingdom*>& kingdoms) {
        spawnMonsters(dt, clock, world);
        processCombat(dt, settlements, kingdoms, world);
    }
    [[nodiscard]] std::size_t getActiveMonsterCount() const noexcept { std::lock_guard<std::mutex> lock(mutex_); return monsters_.size(); }
private:
    mutable std::mutex mutex_;
    std::vector<Monster> monsters_;
    uint32_t nextId_ = 1;
    float spawnAccumulator_ = 0.0f;
    void spawnMonsters(float dt, const WorldClock& clock, voxel::World& world) {
        if (!clock.isNight()) return;
        spawnAccumulator_ += dt * 2.0f;
        std::lock_guard<std::mutex> lock(mutex_);
        while (spawnAccumulator_ >= 1.0f) {
            spawnAccumulator_ -= 1.0f;
            int wx = (rand() % 200) - 100, wz = (rand() % 200) - 100;
            if (world.generator().computeBiome(wx, wz) == voxel::BiomeType::DarkMountainous) {
                MonsterType t = static_cast<MonsterType>(rand() % 3);
                monsters_.push_back({nextId_++, t, glm::vec3(wx, 80.0f, wz), (t == MonsterType::ApostleSpawn) ? 200 : 100, (t == MonsterType::ApostleSpawn) ? 50 : 30, 0});
            }
        }
    }
    void processCombat(float dt, std::vector<economy::Settlement*>& settlements, std::vector<politics::Kingdom*>& kingdoms, voxel::World& world) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = monsters_.begin(); it != monsters_.end(); ) {
            Monster& m = *it;
            if (m.targetSettlementId == 0 && !settlements.empty()) m.targetSettlementId = settlements[0]->id();
            bool died = false;
            for (auto* s : settlements) {
                if (s->id() == m.targetSettlementId) {
                    m.health -= static_cast<int>(10.0f * dt);
                    if (m.health <= 0) { onMonsterDeath(m, world); died = true; break; }
                    for (auto* k : kingdoms) { for (auto* ks : k->settlements()) { if (ks->id() == s->id()) { k->adjustMilitaryPower(-static_cast<int>(m.attackPower * dt * 0.1f)); break; } } }
                    break;
                }
            }
            if (died) it = monsters_.erase(it); else ++it;
        }
    }
    void onMonsterDeath(const Monster& m, voxel::World& world) {
        glm::ivec3 pos = glm::floor(m.position);
        if (voxel::isAir(world.getVoxel(pos.x, pos.y, pos.z).type())) world.setVoxel(pos.x, pos.y, pos.z, voxel::Voxel(voxel::VoxelType::CorruptedStone, 5));
    }
};

} // namespace darkness


// ============================================================================
// 9. ECONOMY PROFESSION SYSTEM & SETTLEMENT IMPLEMENTATIONS (namespace economy)
// ============================================================================
namespace economy {

class ProfessionSystem {
public:
    static void executeProduction(NPC& npc, Settlement& settlement, Market& market, voxel::World& world, float dt) {
        npc.tickCooldown(dt);
        if (npc.productionCooldown() > 0.0f || npc.profession() == NPCProfession::None) return;
        switch (npc.profession()) {
            case NPCProfession::Farmer: tickFarmer(npc, market, world); break;
            case NPCProfession::Miner: tickMiner(npc, market, world); break;
            case NPCProfession::Blacksmith: tickBlacksmith(npc, market, world); break;
            case NPCProfession::Alchemist: tickAlchemist(npc, settlement, market, world); break;
            default: break;
        }
    }
private:
    static void tickFarmer(NPC& npc, Market& market, const voxel::World& world) {
        int fertileCount = countFertileBlocks(world, npc.position(), 2);
        if (fertileCount > 0) market.addSupply(Commodity::Food, std::min(fertileCount, 5));
        npc.resetCooldown();
    }
    static void tickMiner(NPC& npc, Market& market, voxel::World& world) {
        voxel::VoxelType oreType; glm::ivec3 orePos;
        if (findOreNearby(world, npc.position(), 2, oreType, orePos)) {
            world.setVoxel(orePos.x, orePos.y, orePos.z, voxel::Voxel(voxel::VoxelType::Air, 0));
            Commodity produced = Commodity::Stone;
            if (oreType == voxel::VoxelType::Iron_Ore) produced = Commodity::IronOre;
            else if (oreType == voxel::VoxelType::Gold_Ore) produced = Commodity::GoldOre;
            else if (oreType == voxel::VoxelType::CorruptedStone) produced = Commodity::MonsterEssence;
            market.addSupply(produced, 1);
        }
        npc.resetCooldown();
    }
    static void tickBlacksmith(NPC& npc, Market& market, const voxel::World& world) {
        if (market.getSupply(Commodity::IronOre) < 1 || market.getSupply(Commodity::Wood) < 1) { npc.setCooldown(5.0f); return; }
        if (!market.buyItem(npc, Commodity::IronOre, 1)) { npc.setCooldown(5.0f); return; }
        if (!market.buyItem(npc, Commodity::Wood, 1)) { 
            (void)market.sellItem(npc, Commodity::IronOre, 1); 
            npc.setCooldown(5.0f); 
            return; 
        }
        Commodity output = (market.getPrice(Commodity::IronOre) < commodityBasePrice(Commodity::IronOre) * 0.5f) ? Commodity::Tools : Commodity::Weapons;
        (void)market.sellItem(npc, output, 1);
        npc.resetCooldown();
    }
    static void tickAlchemist(NPC& npc, Settlement& settlement, Market& market, const voxel::World& world) {
        if (getBiomeAt(world, npc.position()) != voxel::BiomeType::DarkMountainous) { npc.setCooldown(10.0f); return; }
        if (market.getSupply(Commodity::GoldOre) < 3 || market.getSupply(Commodity::MonsterEssence) < 3) { npc.setCooldown(5.0f); return; }
        (void)darkness::MagicCrafting::craftPotion(npc, Commodity::Potion_BerserkRage, settlement, world);
        npc.resetCooldown();
    }

    static int countFertileBlocks(const voxel::World& world, glm::ivec3 center, int radius) noexcept {
        int count = 0;
        for (int dx = -radius; dx <= radius; ++dx) for (int dz = -radius; dz <= radius; ++dz) {
            if (world.getVoxel(center.x + dx, center.y - 1, center.z + dz).isFertile()) ++count;
        }
        return count;
    }
    static bool findOreNearby(const voxel::World& world, glm::ivec3 center, int radius, voxel::VoxelType& outType, glm::ivec3& outPos) {
        for (int dy = 0; dy >= -3; --dy) for (int dx = -radius; dx <= radius; ++dx) for (int dz = -radius; dz <= radius; ++dz) {
            glm::ivec3 pos = center + glm::ivec3(dx, dy, dz);
            voxel::Voxel v = world.getVoxel(pos.x, pos.y, pos.z);
            if (v.type() == voxel::VoxelType::Iron_Ore || v.type() == voxel::VoxelType::Gold_Ore || v.type() == voxel::VoxelType::Stone || v.type() == voxel::VoxelType::CorruptedStone) { outType = v.type(); outPos = pos; return true; }
        }
        return false;
    }
    static voxel::BiomeType getBiomeAt(const voxel::World& world, glm::ivec3 pos) noexcept { return world.generator().computeBiome(pos.x, pos.z); }
};

void Settlement::updateEconomy(float dt, voxel::World& world) {
    market_.tick(dt);
    tickPassiveResources(dt);
    for (auto& npc : npcs_) ProfessionSystem::executeProduction(*npc, *this, market_, world, dt);
    constructionManager_.tickConstruction(*this, world, dt);
}

void Settlement::tickPassiveResources(float dt) {
    switch (biome_) {
        case voxel::BiomeType::Forest: woodAccumulator_ += dt * 0.5f; if (woodAccumulator_ >= 1.0f) { int u = static_cast<int>(woodAccumulator_); market_.addSupply(Commodity::Wood, u); woodAccumulator_ -= u; } break;
        case voxel::BiomeType::DarkMountainous: stoneAccumulator_ += dt * 0.3f; if (stoneAccumulator_ >= 1.0f) { int u = static_cast<int>(stoneAccumulator_); market_.addSupply(Commodity::Stone, u); stoneAccumulator_ -= u; } break;
        default: break;
    }
}

} // namespace economy


// ============================================================================
// 10. POLITICS OUT-OF-LINE IMPLEMENTATIONS (namespace politics)
// ============================================================================
namespace politics {

int Kingdom::getTotalCommoditySupply(economy::Commodity c) const {
    int t = 0; for (const auto* s : settlements_) t += s->market().getSupply(c); return t;
}
int Kingdom::getAverageCommodityPrice(economy::Commodity c) const {
    if (settlements_.empty()) return 0; int t = 0; for (const auto* s : settlements_) t += s->market().getPrice(c); return t / static_cast<int>(settlements_.size());
}

void KingdomAI::evaluateStrategicGoals(Kingdom& self, const std::vector<Kingdom*>& all, DiplomacyMatrix& d) { evaluateResourceWar(self, all, d); evaluateAlliances(self, all, d); }
void KingdomAI::evaluateResourceWar(Kingdom& self, const std::vector<Kingdom*>& all, DiplomacyMatrix& d) {
    int avgPrice = self.getAverageCommodityPrice(economy::Commodity::IronOre);
    if (avgPrice < economy::commodityBasePrice(economy::Commodity::IronOre) * 3 || self.militaryPower() < 200) return;
    economy::Settlement* raidTarget = nullptr; Kingdom* targetKingdom = nullptr; int maxSupply = 0;
    for (Kingdom* other : all) {
        if (other->id() == self.id() || d.getState(self.id(), other->id()) == DiplomaticState::Alliance) continue;
        for (auto* s : other->settlements()) { int sup = s->market().getSupply(economy::Commodity::IronOre); if (sup > maxSupply) { maxSupply = sup; raidTarget = s; targetKingdom = other; } }
    }
    if (targetKingdom && raidTarget) {
        if (d.getState(self.id(), targetKingdom->id()) != DiplomaticState::War) { d.setState(self.id(), targetKingdom->id(), DiplomaticState::War); self.setTradeDisrupted(true); targetKingdom->setTradeDisrupted(true); std::cout << "[Politics] " << self.name() << " declared war on " << targetKingdom->name() << " for IronOre!\n"; }
        launchSimulatedSiege(self, *targetKingdom, *raidTarget, d);
    }
}
void KingdomAI::evaluateAlliances(Kingdom& self, const std::vector<Kingdom*>& all, DiplomacyMatrix& d) {
    if (self.militaryPower() > 100 || self.treasury() < 500) return;
    for (Kingdom* other : all) { if (other->id() == self.id()) continue; if (d.getState(self.id(), other->id()) == DiplomaticState::Peace && other->militaryPower() > 300) { d.setState(self.id(), other->id(), DiplomaticState::Alliance); std::cout << "[Politics] " << self.name() << " allied with " << other->name() << ".\n"; return; } }
}
void KingdomAI::launchSimulatedSiege(Kingdom& self, Kingdom& target, economy::Settlement& s, DiplomacyMatrix& d) {
    int att = self.militaryPower() + self.treasury() / 10, def = target.militaryPower() + 50;
    self.adjustMilitaryPower(-50); self.adjustTreasury(-200);
    if (att > def) { target.adjustMilitaryPower(-100); int loot = s.market().getSupply(economy::Commodity::IronOre) / 2; s.market().removeSupply(economy::Commodity::IronOre, loot); if (!self.settlements().empty()) self.settlements()[0]->market().addSupply(economy::Commodity::IronOre, loot); std::cout << "[Politics] " << self.name() << " raided " << target.name() << " (+ " << loot << " Iron).\n"; }
    else { self.adjustMilitaryPower(-100); target.adjustMilitaryPower(-30); std::cout << "[Politics] " << self.name() << " repelled by " << target.name() << ".\n"; }
}

void ReputationManager::modifyReputation(uint32_t pid, uint32_t kid, int amount) { std::unique_lock<std::shared_mutex> lock(mutex_); int& val = map_[pid][kid]; val = std::clamp(val + amount, kMin, kMax); }
int ReputationManager::getReputation(uint32_t pid, uint32_t kid) const { std::shared_lock<std::shared_mutex> lock(mutex_); auto pIt = map_.find(pid); if (pIt == map_.end()) return 0; auto kIt = pIt->second.find(kid); return (kIt != pIt->second.end()) ? kIt->second : 0; }
ReputationTier ReputationManager::getTier(uint32_t pid, uint32_t kid) const { int r = getReputation(pid, kid); if (r >= 750) return ReputationTier::Exalted; if (r >= 250) return ReputationTier::Friendly; if (r <= -750) return ReputationTier::SwornEnemy; if (r <= -250) return ReputationTier::Hostile; return ReputationTier::Neutral; }
void ReputationManager::reportPlayerAction(uint32_t pid, Kingdom& target, std::string_view action, const std::vector<Kingdom*>& all, DiplomacyMatrix& d) {
    if (action == "destroy_voxel") modifyReputation(pid, target, -5);
    else if (action == "raid_village") { modifyReputation(pid, target, -250); target.setTradeDisrupted(true); }
    else if (action == "destroy_settlement") {
        modifyReputation(pid, target, -1000); target.setTradeDisrupted(true);
        std::cout << "[Politics] Player destroyed " << target.name() << "'s settlement! Now Sworn Enemy.\n";
        for (Kingdom* other : all) { if (other->id() == target.id()) continue; if (d.getState(target.id(), other->id()) == DiplomaticState::War) { modifyReputation(pid, *other, +500); std::cout << "[Politics] Rival " << other->name() << " rejoices! (+500 Rep)\n"; } }
    }
}

} // namespace politics


// ============================================================================
// 11. CONSTRUCTION OUT-OF-LINE IMPLEMENTATIONS (namespace construction)
// ============================================================================
namespace construction {

void ConstructionManager::queueStructureBlueprint(ZoneType type, glm::ivec3 origin) {
    ConstructionZone z; z.id = static_cast<uint32_t>(zones_.size() + 1); z.type = type; z.origin = origin;
    if (type == ZoneType::Residential) generateResidentialBlueprint(z);
    else if (type == ZoneType::Military) generateMilitaryBlueprint(z);
    if (z.totalVoxels > 0) zones_.push_back(std::move(z));
}
void ConstructionManager::generateResidentialBlueprint(ConstructionZone& z) {
    for (int x = 0; x < 5; ++x) for (int zc = 0; zc < 5; ++zc) for (int y = 0; y < 4; ++y)
        if (x == 0 || x == 4 || zc == 0 || zc == 4 || y == 3) z.targetVoxels.push_back({glm::ivec3(x, y, zc), voxel::VoxelType::Wood});
    z.totalVoxels = static_cast<int>(z.targetVoxels.size());
}
void ConstructionManager::generateMilitaryBlueprint(ConstructionZone& z) {
    for (int x = 0; x < 10; ++x) for (int y = 0; y < 3; ++y) z.targetVoxels.push_back({glm::ivec3(x, y, 0), voxel::VoxelType::Stone});
    z.totalVoxels = static_cast<int>(z.targetVoxels.size());
}
void ConstructionManager::tickConstruction(economy::Settlement& settlement, voxel::World& world, float dt) {
    int placements = 0;
    for (auto& zone : zones_) {
        if (zone.isComplete || placements >= kMaxPlacementsPerTick) continue;
        if (zone.assignedBuilderId == 0) {
            for (auto& npc : settlement.npcs()) {
                if (npc->profession() == economy::NPCProfession::None && assignedNpcs_.find(npc->id()) == assignedNpcs_.end()) { zone.assignedBuilderId = npc->id(); assignedNpcs_.insert(npc->id()); break; }
            }
        }
        if (zone.assignedBuilderId == 0) continue;
        zone.buildTimer -= dt; if (zone.buildTimer > 0.0f) continue;
        TargetVoxel target = zone.targetVoxels.back();
        economy::Commodity req = (target.type == voxel::VoxelType::Stone) ? economy::Commodity::Stone : economy::Commodity::Wood;
        if (settlement.market().getSupply(req) > 0) {
            settlement.market().removeSupply(req, 1);
            glm::ivec3 wp = zone.origin + target.localPos;
            world.setVoxel(wp.x, wp.y, wp.z, voxel::Voxel(target.type, 7));
            zone.targetVoxels.pop_back(); zone.progress++; zone.buildTimer = 2.0f; placements++;
            if (zone.targetVoxels.empty()) { zone.isComplete = true; assignedNpcs_.erase(zone.assignedBuilderId); zone.assignedBuilderId = 0; onZoneComplete(settlement, world, zone); }
        } else { zone.buildTimer = 5.0f; }
    }
}
void ConstructionManager::onZoneComplete(economy::Settlement& s, voxel::World& w, const ConstructionZone& z) {
    if (z.type == ZoneType::Residential) { s.adjustMaxNPCs(4); std::cout << "[Construction] House built. Capacity +4.\n"; }
    else if (z.type == ZoneType::Agricultural) {
        int fertile = 0; for (int x = 0; x < 5; ++x) for (int zc = 0; zc < 5; ++zc) if (w.getVoxel(z.origin.x + x, z.origin.y - 1, z.origin.z + zc).isFertile()) fertile++;
        if (fertile > 15) { s.adjustAgricultureBonus(10.0f); std::cout << "[Construction] Farm built on fertile soil!\n"; }
    }
}
bool ConstructionManager::buyOutConstruction(uint32_t zoneId, politics::Kingdom& k, voxel::World& w) {
    auto it = std::find_if(zones_.begin(), zones_.end(), [zoneId](const ConstructionZone& z) { return z.id == zoneId; });
    if (it == zones_.end() || it->isComplete) return false;
    int cost = static_cast<int>(it->targetVoxels.size()) * 50;
    if (k.treasury() < cost) return false;
    k.adjustTreasury(-cost);
    for (const auto& t : it->targetVoxels) { glm::ivec3 wp = it->origin + t.localPos; w.setVoxel(wp.x, wp.y, wp.z, voxel::Voxel(t.type, 7)); it->progress++; }
    it->targetVoxels.clear(); it->isComplete = true;
    if (it->assignedBuilderId != 0) assignedNpcs_.erase(it->assignedBuilderId);
    std::cout << "[Construction] Zone " << zoneId << " bought out instantly!\n";
    return true;
}

} // namespace construction


// ============================================================================
// 12. RAYLIB FRONTEND: RENDERING & UI HELPERS
// ============================================================================

Color GetVoxelColor(voxel::VoxelType type) {
    switch(type) {
        case voxel::VoxelType::Grass: return (Color){ 50, 220, 50, 255 };
        case voxel::VoxelType::Dirt: return (Color){ 150, 90, 50, 255 };
        case voxel::VoxelType::Stone: return (Color){ 120, 120, 120, 255 };
        case voxel::VoxelType::Wood: return (Color){ 100, 60, 30, 255 };
        case voxel::VoxelType::CorruptedStone: return (Color){ 180, 0, 255, 255 };
        case voxel::VoxelType::Water: return (Color){ 0, 100, 220, 180 };
        case voxel::VoxelType::Sand: return (Color){ 240, 210, 110, 255 };
        case voxel::VoxelType::Sandstone: return (Color){ 200, 170, 80, 255 };
        case voxel::VoxelType::Iron_Ore: return (Color){ 200, 150, 100, 255 };
        case voxel::VoxelType::Gold_Ore: return (Color){ 255, 215, 0, 255 };
        case voxel::VoxelType::Bedrock: return (Color){ 40, 40, 40, 255 };
        default: return BLACK;
    }
}

void UpdateRTSCamera(Camera3D &camera) {
    Vector2 mouseDelta = GetMouseDelta();
    float wheel = GetMouseWheelMove();

    if (wheel != 0) {
        Vector3 dir = (Vector3){ camera.target.x - camera.position.x, camera.target.y - camera.position.y, camera.target.z - camera.position.z };
        float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
        if (len > 0) {
            dir.x /= len; dir.y /= len; dir.z /= len;
            camera.position.x += dir.x * wheel * 5.0f;
            camera.position.y += dir.y * wheel * 5.0f;
            camera.position.z += dir.z * wheel * 5.0f;
        }
    }

    Vector3 forward = (Vector3){ camera.target.x - camera.position.x, 0.0f, camera.target.z - camera.position.z };
    Vector3 right = (Vector3){ forward.z, 0.0f, -forward.x };
    float fwdLen = sqrtf(forward.x*forward.x + forward.z*forward.z);
    if (fwdLen > 0) { forward.x /= fwdLen; forward.z /= fwdLen; }
    float rightLen = sqrtf(right.x*right.x + right.z*right.z);
    if (rightLen > 0) { right.x /= rightLen; right.z /= rightLen; }

    float speed = 0.5f;
    if (IsKeyDown(KEY_W)) { camera.position.x += forward.x * speed; camera.position.z += forward.z * speed; camera.target.x += forward.x * speed; camera.target.z += forward.z * speed; }
    if (IsKeyDown(KEY_S)) { camera.position.x -= forward.x * speed; camera.position.z -= forward.z * speed; camera.target.x -= forward.x * speed; camera.target.z -= forward.z * speed; }
    if (IsKeyDown(KEY_A)) { camera.position.x -= right.x * speed; camera.position.z -= right.z * speed; camera.target.x -= right.x * speed; camera.target.z -= right.z * speed; }
    if (IsKeyDown(KEY_D)) { camera.position.x += right.x * speed; camera.position.z += right.z * speed; camera.target.x += right.x * speed; camera.target.z += right.z * speed; }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        float angle = mouseDelta.x * 0.01f;
        float dx = camera.position.x - camera.target.x;
        float dz = camera.position.z - camera.target.z;
        float newX = dx * cosf(angle) - dz * sinf(angle);
        float newZ = dx * sinf(angle) + dz * cosf(angle);
        camera.position.x = camera.target.x + newX;
        camera.position.z = camera.target.z + newZ;
    }
    camera.target.y = 64.0f;
}

void DrawVoxelWorld(voxel::World& world, int centerX, int centerZ) {
    int renderRadius = 15;
    for (int x = -renderRadius; x <= renderRadius; ++x) {
        for (int z = -renderRadius; z <= renderRadius; ++z) {
            int wx = centerX + x;
            int wz = centerZ + z;
            
            int surfaceY = -1;
            for (int y = 127; y >= 0; --y) {
                voxel::Voxel v = world.getVoxel(wx, y, wz);
                if (voxel::isSolid(v.type())) {
                    surfaceY = y;
                    break;
                }
            }
            
            if (surfaceY != -1) {
                for (int dy = 0; dy < 3; ++dy) {
                    int y = surfaceY - dy;
                    if (y < 0) break;
                    voxel::Voxel v = world.getVoxel(wx, y, wz);
                    Color c = GetVoxelColor(v.type());
                    Vector3 pos = (Vector3){ (float)wx + 0.5f, (float)y + 0.5f, (float)wz + 0.5f };
                    DrawCube(pos, 1.0f, 1.0f, 1.0f, c);
                    DrawCubeWires(pos, 1.0f, 1.0f, 1.0f, (Color){30, 30, 30, 255});
                }
            }
        }
    }
}

bool DrawButton(Rectangle bounds, const char* text, Color bg, Color fg) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    
    DrawRectangleRec(bounds, hovered ? Fade(bg, 0.9f) : bg);
    DrawRectangleLinesEx(bounds, 2, BLACK);
    int textWidth = MeasureText(text, 20);
    DrawText(text, bounds.x + (bounds.width - textWidth)/2, bounds.y + (bounds.height - 20)/2, 20, fg);
    return clicked;
}

void DrawUI(economy::Settlement& settlement, politics::Kingdom& kingdom, darkness::WorldClock& clock, bool& placingResidential, bool& placingMilitary) {
    DrawRectangle(0, 0, GetScreenWidth(), 40, Fade(BLACK, 0.7f));
    DrawText(TextFormat("Time: %02.0f:00 | Kingdom: %s | Treasury: %dG | Military: %d", 
              clock.getTime(), kingdom.name().c_str(), kingdom.treasury(), kingdom.militaryPower()), 10, 10, 20, RAYWHITE);

    DrawRectangle(0, 40, 220, GetScreenHeight()-40, Fade(BLACK, 0.5f));
    DrawText("MARKET", 10, 50, 20, RAYWHITE);
    int y = 80;
    const char* itemNames[] = {"Food", "Wood", "Iron", "Weapons", "Potions"};
    economy::Commodity types[] = {economy::Commodity::Food, economy::Commodity::Wood, economy::Commodity::IronOre, economy::Commodity::Weapons, economy::Commodity::Potion_FireResist};
    
    for(int i=0; i<5; i++) {
        DrawText(TextFormat("%s: %dG (S:%d)", itemNames[i], settlement.market().getPrice(types[i]), settlement.market().getSupply(types[i])), 10, y, 18, RAYWHITE);
        if (DrawButton((Rectangle){130.0f, (float)y - 2.0f, 40.0f, 22.0f}, "BUY", (Color){50, 50, 150, 255}, WHITE)) {
            settlement.market().addSupply(types[i], 1);
        }
        if (DrawButton((Rectangle){175.0f, (float)y - 2.0f, 40.0f, 22.0f}, "SELL", (Color){150, 50, 50, 255}, WHITE)) {
            settlement.market().removeSupply(types[i], 1);
        }
        y += 30;
    }

    DrawRectangle(GetScreenWidth()-220, 40, 220, GetScreenHeight()-40, Fade(BLACK, 0.5f));
    DrawText("BUILD", GetScreenWidth()-210, 50, 20, RAYWHITE);
    
    if (DrawButton((Rectangle){(float)(GetScreenWidth()-210), 80.0f, 200.0f, 30.0f}, "Residential House", BROWN, WHITE)) {
        placingResidential = true; placingMilitary = false;
    }
    if (DrawButton((Rectangle){(float)(GetScreenWidth()-210), 120.0f, 200.0f, 30.0f}, "Military Barracks", GRAY, WHITE)) {
        placingMilitary = true; placingResidential = false;
    }
}

void DrawPlacementPreview(Camera3D camera, bool& placingResidential, bool& placingMilitary, economy::Settlement& settlement) {
    if (!placingResidential && !placingMilitary) return;

    Vector2 mousePos = GetMousePosition();
    if (mousePos.x < 220 || mousePos.x > GetScreenWidth() - 220 || mousePos.y < 40) return;

    Ray ray = GetScreenToWorldRay(mousePos, camera);
    Vector3 targetPos = (Vector3){0.0f, 0.0f, 0.0f};
    float planeY = 64.0f;
    if (ray.direction.y != 0) {
        float t = (planeY - ray.position.y) / ray.direction.y;
        if (t > 0) {
            targetPos.x = ray.position.x + ray.direction.x * t;
            targetPos.y = planeY;
            targetPos.z = ray.position.z + ray.direction.z * t;
        }
    }
    int gridX = (int)floorf(targetPos.x);
    int gridZ = (int)floorf(targetPos.z);
    
    Vector3 previewPos = (Vector3){ (float)gridX + 2.5f, planeY + 2.0f, (float)gridZ + 2.5f };
    DrawCubeWires(previewPos, 5.0f, 4.0f, 5.0f, GREEN);
    
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        glm::ivec3 origin = glm::ivec3(gridX, (int)planeY, gridZ);
        if (placingResidential) {
            settlement.constructionManager().queueStructureBlueprint(construction::ZoneType::Residential, origin);
        } else {
            settlement.constructionManager().queueStructureBlueprint(construction::ZoneType::Military, origin);
        }
        placingResidential = false;
        placingMilitary = false;
    }
}


// ============================================================================
// 13. MAIN EXECUTION LOOP (Raylib App Entry Point)
// ============================================================================

int main() {
    InitWindow(1280, 720, "Dark Fantasy Voxel Strategy - Raylib");
    SetTargetFPS(60);

    voxel::World world(1337);
    world.ensureChunksAround(0, 0, 1);

    economy::Settlement settlement(1, glm::ivec3(0, 70, 0), voxel::BiomeType::Forest);
    politics::Kingdom kingdom(1, "Midland", 1);
    kingdom.addSettlement(&settlement);
    politics::DiplomacyMatrix diplomacy(1);

    darkness::WorldClock clock;
    darkness::MonsterManager monsterManager;

    std::vector<politics::Kingdom*> kingdoms = {&kingdom};
    std::vector<economy::Settlement*> settlements = {&settlement};

    auto farmer = std::make_unique<economy::NPC>(1, economy::NPCProfession::Farmer, glm::ivec3(0, 70, 0));
    auto miner = std::make_unique<economy::NPC>(2, economy::NPCProfession::Miner, glm::ivec3(0, 70, 0));
    settlement.addNPC(std::move(farmer));
    settlement.addNPC(std::move(miner));

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 30.0f, 40.0f, 30.0f };
    camera.target = (Vector3){ 0.0f, 64.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    bool placingResidential = false;
    bool placingMilitary = false;
    float simAccumulator = 0.0f;
    float simTickRate = 0.1f;

    while (!WindowShouldClose()) {
        UpdateRTSCamera(camera);

        float dt = GetFrameTime();
        simAccumulator += dt;
        while (simAccumulator >= simTickRate) {
            clock.tick(simTickRate);
            settlement.updateEconomy(simTickRate, world);
            monsterManager.tickMonsters(simTickRate, clock, world, settlements, kingdoms);
            simAccumulator -= simTickRate;
        }

        Color bgColor = clock.isNight() ? (Color){ 15, 0, 5, 255 } : (Color){ 135, 206, 235, 255 };
        BeginDrawing();
        ClearBackground(bgColor);

        BeginMode3D(camera);
            DrawVoxelWorld(world, 0, 0);
            DrawPlacementPreview(camera, placingResidential, placingMilitary, settlement);
        EndMode3D();

        DrawUI(settlement, kingdom, clock, placingResidential, placingMilitary);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}