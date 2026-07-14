#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <string>
#include <map>
#include <algorithm>

// ============================================================
// Helper: portable Color construction
// ============================================================
inline Color MKCOL(int r, int g, int b, int a = 255) {
    Color c;
    c.r = (unsigned char)r; c.g = (unsigned char)g;
    c.b = (unsigned char)b; c.a = (unsigned char)a;
    return c;
}

// Custom cube wires to bypass Raylib's matrix stack overflow bug
inline void drawCubeWiresCustom(Vector3 pos, float w, float h, float l, Color c) {
    float x = pos.x, y = pos.y, z = pos.z;
    float hw = w / 2.0f, hh = h / 2.0f, hl = l / 2.0f;
    DrawLine3D({x - hw, y - hh, z - hl}, {x + hw, y - hh, z - hl}, c);
    DrawLine3D({x + hw, y - hh, z - hl}, {x + hw, y - hh, z + hl}, c);
    DrawLine3D({x + hw, y - hh, z + hl}, {x - hw, y - hh, z + hl}, c);
    DrawLine3D({x - hw, y - hh, z + hl}, {x - hw, y - hh, z - hl}, c);

    DrawLine3D({x - hw, y + hh, z - hl}, {x + hw, y + hh, z - hl}, c);
    DrawLine3D({x + hw, y + hh, z - hl}, {x + hw, y + hh, z + hl}, c);
    DrawLine3D({x + hw, y + hh, z + hl}, {x - hw, y + hh, z + hl}, c);
    DrawLine3D({x - hw, y + hh, z + hl}, {x - hw, y + hh, z - hl}, c);

    DrawLine3D({x - hw, y - hh, z - hl}, {x - hw, y + hh, z - hl}, c);
    DrawLine3D({x + hw, y - hh, z - hl}, {x + hw, y + hh, z - hl}, c);
    DrawLine3D({x + hw, y - hh, z + hl}, {x + hw, y + hh, z + hl}, c);
    DrawLine3D({x - hw, y - hh, z + hl}, {x - hw, y + hh, z + hl}, c);
}

// ============================================================
// Constants
// ============================================================
constexpr int WDX = 64;
constexpr int WDY = 32;
constexpr int WDZ = 64;

constexpr float PLAYER_WIDTH  = 0.6f;
constexpr float PLAYER_HEIGHT = 1.7f;
constexpr float EYE_HEIGHT    = 1.6f;
constexpr float GRAVITY       = 25.0f;
constexpr float JUMP_SPEED    = 8.5f;
constexpr float MOVE_SPEED    = 5.0f;
constexpr float SPRINT_SPEED  = 8.5f;
constexpr float MOUSE_SENS    = 0.0025f;
constexpr float REACH         = 6.0f;
constexpr float DAY_LENGTH    = 120.0f;

// ============================================================
// Block types
// ============================================================
enum BlockType : unsigned char {
    BL_AIR = 0, BL_GRASS, BL_DIRT, BL_STONE, BL_WATER,
    BL_CROP, BL_WOOD, BL_LEAF, BL_SAND, BL_PLANK
};

inline bool blockIsSolid(BlockType b) { return b != BL_AIR && b != BL_WATER; }

inline Color blockColor(BlockType b) {
    switch (b) {
        case BL_GRASS: return MKCOL(80, 160, 60);
        case BL_DIRT:  return MKCOL(120, 80, 50);
        case BL_STONE: return MKCOL(130, 130, 135);
        case BL_WATER: return MKCOL(50, 100, 200, 180);
        case BL_CROP:  return MKCOL(220, 200, 60);
        case BL_WOOD:  return MKCOL(100, 70, 40);
        case BL_LEAF:  return MKCOL(60, 140, 50);
        case BL_SAND:  return MKCOL(220, 200, 140);
        case BL_PLANK: return MKCOL(160, 110, 60);
        default: return BLACK;
    }
}

inline const char* blockName(BlockType b) {
    switch (b) {
        case BL_GRASS: return "Grass";
        case BL_DIRT:  return "Dirt";
        case BL_STONE: return "Stone";
        case BL_WATER: return "Water";
        case BL_CROP:  return "Crop";
        case BL_WOOD:  return "Wood";
        case BL_LEAF:  return "Leaf";
        case BL_SAND:  return "Sand";
        case BL_PLANK: return "Plank";
        default: return "Air";
    }
}

const BlockType HOTBAR[] = { BL_GRASS, BL_DIRT, BL_STONE, BL_WOOD, BL_PLANK, BL_SAND, BL_CROP, BL_WATER };
constexpr int HOTBAR_SIZE = 8;

// ============================================================
// Voxel World
// ============================================================
struct ExposedBlock { int x, y, z; BlockType b; };

struct VoxelWorld {
    std::vector<BlockType> blocks;
    std::vector<ExposedBlock> exposedBlocks;
    bool dirty = true;

    VoxelWorld() { blocks.resize((size_t)WDX * WDY * WDZ, BL_AIR); }

    int idx(int x, int y, int z) const {
        if (x < 0 || x >= WDX || y < 0 || y >= WDY || z < 0 || z >= WDZ) return -1;
        return x + WDX * y + (size_t)WDX * WDY * z;
    }

    BlockType get(int x, int y, int z) const {
        int i = idx(x, y, z);
        return (i < 0) ? BL_AIR : blocks[i];
    }

    void set(int x, int y, int z, BlockType b) {
        int i = idx(x, y, z);
        if (i < 0) return;
        if (blocks[i] != b) { blocks[i] = b; dirty = true; }
    }

    bool isSolidAt(int x, int y, int z) const { return blockIsSolid(get(x, y, z)); }

    int surfaceHeight(int x, int z) const {
        for (int y = WDY - 1; y >= 0; y--) {
            BlockType b = get(x, y, z);
            if (b != BL_AIR && b != BL_WATER) return y;
        }
        return 0;
    }

    bool hasWaterNeighbor(int x, int y, int z) const {
        return get(x+1,y,z) == BL_WATER || get(x-1,y,z) == BL_WATER ||
               get(x,y,z+1) == BL_WATER || get(x,y,z-1) == BL_WATER;
    }

    void generate() {
        for (int x = 0; x < WDX; x++) {
            for (int z = 0; z < WDZ; z++) {
                float h = 9.0f + 4.0f * sinf(x * 0.15f) * cosf(z * 0.12f)
                              + 2.0f * sinf((x + z) * 0.08f);
                int height = (int)h;
                if (height < 1) height = 1;
                if (height >= WDY - 2) height = WDY - 3;

                for (int y = 0; y <= height; y++) {
                    if (y == height) {
                        set(x, y, z, (height <= 8) ? BL_SAND : BL_GRASS);
                    } else if (y > height - 3) {
                        set(x, y, z, BL_DIRT);
                    } else {
                        set(x, y, z, BL_STONE);
                    }
                }
                if (height < 8) {
                    for (int y = height + 1; y <= 8; y++) {
                        if (get(x, y, z) == BL_AIR) set(x, y, z, BL_WATER);
                    }
                }
            }
        }
        for (int i = 0; i < 25; i++) {
            int tx = GetRandomValue(4, WDX - 5);
            int tz = GetRandomValue(4, WDZ - 5);
            int ty = -1;
            for (int y = WDY - 1; y >= 0; y--) {
                if (get(tx, y, tz) == BL_GRASS) { ty = y; break; }
            }
            if (ty < 0) continue;
            int trunkH = 3 + GetRandomValue(0, 2);
            for (int y = ty + 1; y <= ty + trunkH; y++) set(tx, y, tz, BL_WOOD);
            for (int dx = -2; dx <= 2; dx++)
                for (int dz = -2; dz <= 2; dz++)
                    for (int dy = 0; dy <= 2; dy++)
                        if (dx*dx + dz*dz + (dy-1)*(dy-1) <= 5) {
                            int lx = tx+dx, ly = ty+trunkH+dy, lz = tz+dz;
                            if (get(lx, ly, lz) == BL_AIR) set(lx, ly, lz, BL_LEAF);
                        }
        }
        for (int i = 0; i < 40; i++) {
            int cx = GetRandomValue(18, 46);
            int cz = GetRandomValue(18, 46);
            int sy = surfaceHeight(cx, cz);
            if (get(cx, sy, cz) == BL_GRASS) set(cx, sy + 1, cz, BL_CROP);
        }
        dirty = true;
    }

    bool isExposed(int x, int y, int z) const {
        BlockType b = get(x, y, z);
        if (b == BL_AIR) return false;
        auto seeThrough = [](BlockType nb, BlockType cur) -> bool {
            if (nb == BL_AIR) return true;
            if (nb == BL_WATER && cur != BL_WATER) return true;
            if (nb == BL_LEAF && cur != BL_LEAF) return true;
            return false;
        };
        return seeThrough(get(x+1,y,z), b) || seeThrough(get(x-1,y,z), b) ||
               seeThrough(get(x,y+1,z), b) || seeThrough(get(x,y-1,z), b) ||
               seeThrough(get(x,y,z+1), b) || seeThrough(get(x,y,z-1), b);
    }

    void rebuildExposed() {
        exposedBlocks.clear();
        for (int x = 0; x < WDX; x++)
            for (int y = 0; y < WDY; y++)
                for (int z = 0; z < WDZ; z++) {
                    BlockType b = get(x, y, z);
                    if (b == BL_AIR) continue;
                    if (!isExposed(x, y, z)) continue;
                    exposedBlocks.push_back({x, y, z, b});
                }
        dirty = false;
    }

    void draw() {
        if (dirty) rebuildExposed();
        for (const auto& eb : exposedBlocks) {
            Color c = blockColor(eb.b);
            Vector3 pos = { (float)eb.x + 0.5f, (float)eb.y + 0.5f, (float)eb.z + 0.5f };
            if (eb.b == BL_WATER) {
                DrawCube(pos, 1.0f, 0.9f, 1.0f, c);
            } else if (eb.b == BL_CROP) {
                DrawCube(pos, 0.6f, 0.8f, 0.6f, c);
            } else {
                DrawCube(pos, 1.0f, 1.0f, 1.0f, c);
            }
            drawCubeWiresCustom(pos, 1.0f, 1.0f, 1.0f, MKCOL(0, 0, 0, 40));
        }
    }
};

// ============================================================
// Player
// ============================================================
struct Player {
    Vector3 pos;
    Vector3 vel;
    float yaw = 0, pitch = 0;
    bool onGround = false;
    int gold = 100, wood = 16, stone = 16, food = 10;
    bool hasSword = false, swordEquipped = false;
    int selectedBlock = 0;
    float attackCooldown = 0;
    float health = 100.0f, maxHealth = 100.0f;
    bool inShop = false;
};

bool collidesWorld(const VoxelWorld& world, Vector3 pos) {
    float hw = PLAYER_WIDTH * 0.5f;
    int minX = (int)floorf(pos.x - hw), maxX = (int)floorf(pos.x + hw);
    int minY = (int)floorf(pos.y),         maxY = (int)floorf(pos.y + PLAYER_HEIGHT - 0.001f);
    int minZ = (int)floorf(pos.z - hw), maxZ = (int)floorf(pos.z + hw);
    for (int x = minX; x <= maxX; x++)
        for (int y = minY; y <= maxY; y++)
            for (int z = minZ; z <= maxZ; z++)
                if (world.isSolidAt(x, y, z)) return true;
    return false;
}

void updatePlayer(Player& p, VoxelWorld& world, float dt) {
    Vector2 md = GetMouseDelta();
    p.yaw   -= md.x * MOUSE_SENS;
    p.pitch -= md.y * MOUSE_SENS;
    p.pitch = Clamp(p.pitch, -PI / 2 + 0.01f, PI / 2 - 0.01f);

    float sy = sinf(p.yaw), cy = cosf(p.yaw);
    Vector3 hf    = { -sy, 0, -cy };
    Vector3 right = {  cy, 0, -sy };

    Vector3 moveDir = { 0, 0, 0 };
    if (IsKeyDown(KEY_W)) moveDir = Vector3Add(moveDir, hf);
    if (IsKeyDown(KEY_S)) moveDir = Vector3Subtract(moveDir, hf);
    if (IsKeyDown(KEY_D)) moveDir = Vector3Add(moveDir, right);
    if (IsKeyDown(KEY_A)) moveDir = Vector3Subtract(moveDir, right);

    if (Vector3Length(moveDir) > 0.001f) {
        moveDir = Vector3Normalize(moveDir);
        float speed = IsKeyDown(KEY_LEFT_SHIFT) ? SPRINT_SPEED : MOVE_SPEED;
        p.vel.x = moveDir.x * speed;
        p.vel.z = moveDir.z * speed;
    } else {
        p.vel.x = 0; p.vel.z = 0;
    }

    p.vel.y -= GRAVITY * dt;
    if (p.vel.y < -35.0f) p.vel.y = -35.0f;

    if (IsKeyPressed(KEY_SPACE) && p.onGround) {
        p.vel.y = JUMP_SPEED;
        p.onGround = false;
    }

    Vector3 newPos = p.pos;
    newPos.x += p.vel.x * dt;
    if (collidesWorld(world, newPos)) { newPos.x = p.pos.x; p.vel.x = 0; }
    p.pos.x = newPos.x;

    newPos = p.pos;
    newPos.z += p.vel.z * dt;
    if (collidesWorld(world, newPos)) { newPos.z = p.pos.z; p.vel.z = 0; }
    p.pos.z = newPos.z;

    newPos = p.pos;
    newPos.y += p.vel.y * dt;
    if (collidesWorld(world, newPos)) {
        if (p.vel.y < 0) p.onGround = true;
        p.vel.y = 0;
    } else {
        p.onGround = false;
    }
    p.pos.y = newPos.y;

    p.pos.x = Clamp(p.pos.x, 1.0f, (float)(WDX - 1));
    p.pos.z = Clamp(p.pos.z, 1.0f, (float)(WDZ - 1));
    if (p.pos.y < 0) { p.pos.y = 0; p.vel.y = 0; }

    if (p.attackCooldown > 0) p.attackCooldown -= dt;
}

Vector3 getPlayerEye(const Player& p) {
    return { p.pos.x, p.pos.y + EYE_HEIGHT, p.pos.z };
}
Vector3 getPlayerForward(const Player& p) {
    float cp = cosf(p.pitch), sp = sinf(p.pitch);
    return { -sinf(p.yaw) * cp, sp, -cosf(p.yaw) * cp };
}

// ============================================================
// DDA Voxel Raycast
// ============================================================
bool raycastVoxel(const VoxelWorld& world, Vector3 origin, Vector3 dir, float maxDist,
                  int& hitX, int& hitY, int& hitZ, int& faceX, int& faceY, int& faceZ) {
    int x = (int)floorf(origin.x);
    int y = (int)floorf(origin.y);
    int z = (int)floorf(origin.z);

    int stepX = (dir.x > 0) ? 1 : -1;
    int stepY = (dir.y > 0) ? 1 : -1;
    int stepZ = (dir.z > 0) ? 1 : -1;

    float tDeltaX = (dir.x != 0) ? fabsf(1.0f / dir.x) : 1e30f;
    float tDeltaY = (dir.y != 0) ? fabsf(1.0f / dir.y) : 1e30f;
    float tDeltaZ = (dir.z != 0) ? fabsf(1.0f / dir.z) : 1e30f;

    float tMaxX = 1e30f, tMaxY = 1e30f, tMaxZ = 1e30f;
    if (dir.x > 0) tMaxX = (x + 1 - origin.x) * tDeltaX;
    else if (dir.x < 0) tMaxX = (origin.x - x) * tDeltaX;
    if (dir.y > 0) tMaxY = (y + 1 - origin.y) * tDeltaY;
    else if (dir.y < 0) tMaxY = (origin.y - y) * tDeltaY;
    if (dir.z > 0) tMaxZ = (z + 1 - origin.z) * tDeltaZ;
    else if (dir.z < 0) tMaxZ = (origin.z - z) * tDeltaZ;

    faceX = faceY = faceZ = 0;
    float t = 0;
    while (t <= maxDist) {
        if (world.isSolidAt(x, y, z)) {
            hitX = x; hitY = y; hitZ = z;
            return true;
        }
        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            x += stepX; t = tMaxX; tMaxX += tDeltaX;
            faceX = -stepX; faceY = 0; faceZ = 0;
        } else if (tMaxY < tMaxZ) {
            y += stepY; t = tMaxY; tMaxY += tDeltaY;
            faceX = 0; faceY = -stepY; faceZ = 0;
        } else {
            z += stepZ; t = tMaxZ; tMaxZ += tDeltaZ;
            faceX = 0; faceY = 0; faceZ = -stepZ;
        }
    }
    return false;
}

// ============================================================
// Kingdom, NPC, Monster
// ============================================================
struct Kingdom {
    std::string name;
    int gold;
    int food;
    int reputation;
    Color color;
    Vector3 center;
    float radius;
    float foodTimer;
};

enum NPCType   { NPC_FARMER, NPC_GUARD };
enum NPCState  { NS_IDLE, NS_MOVE_TO_CROP, NS_HARVEST, NS_RETURN_HOME, NS_DEPOSIT, NS_PATROL, NS_ATTACK, NS_FLEE };

struct NPC {
    int id, kingdomId, type;
    Vector3 pos, vel;
    int state;
    float timer;
    Vector3 target;
    float health, maxHealth;
    int carriedFood;
    int targetMonsterId;
    float attackCooldown;
    bool alive;
};

struct Monster {
    int id;
    Vector3 pos, vel;
    float health;
    float attackCooldown;
    bool alive;
    float spawnTime;
};

float findGroundY(const VoxelWorld& world, Vector3 pos) {
    int x = (int)floorf(pos.x);
    int z = (int)floorf(pos.z);
    for (int y = WDY - 1; y >= 0; y--) {
        if (world.isSolidAt(x, y, z)) return (float)(y + 1);
    }
    return 0.0f;
}

void moveNPCOnGround(NPC& npc, Vector3 target, float speed, float dt, const VoxelWorld& world) {
    Vector3 toTarget = { target.x - npc.pos.x, 0, target.z - npc.pos.z };
    float dist = sqrtf(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
    if (dist > 0.1f) {
        npc.vel.x = (toTarget.x / dist) * speed;
        npc.vel.z = (toTarget.z / dist) * speed;
    } else {
        npc.vel.x = 0; npc.vel.z = 0;
    }
    Vector3 newPos = npc.pos;
    newPos.x += npc.vel.x * dt;
    newPos.z += npc.vel.z * dt;

    int by = (int)floorf(npc.pos.y);
    int bx = (int)floorf(newPos.x);
    int bz = (int)floorf(newPos.z);
    bool blocked = world.isSolidAt(bx, by, bz) || world.isSolidAt(bx, by + 1, bz);

    if (!blocked) {
        npc.pos.x = newPos.x;
        npc.pos.z = newPos.z;
    } else {
        int cx = (int)floorf(newPos.x);
        int cz = (int)floorf(npc.pos.z);
        if (!world.isSolidAt(cx, by, cz) && !world.isSolidAt(cx, by + 1, cz)) npc.pos.x = newPos.x;
        int cx2 = (int)floorf(npc.pos.x);
        int cz2 = (int)floorf(newPos.z);
        if (!world.isSolidAt(cx2, by, cz2) && !world.isSolidAt(cx2, by + 1, cz2)) npc.pos.z = newPos.z;
    }

    float groundY = findGroundY(world, npc.pos);
    if (npc.pos.y < groundY) npc.pos.y = groundY;
    else if (npc.pos.y > groundY + 0.1f) {
        npc.pos.y -= 10.0f * dt;
        if (npc.pos.y < groundY) npc.pos.y = groundY;
    } else {
        npc.pos.y = groundY;
    }
}

Vector3 findNearestCrop(const VoxelWorld& world, Vector3 from) {
    Vector3 best = { -1, -1, -1 };
    float bestDist = 1e9f;
    int r = 20;
    int cx = (int)floorf(from.x);
    int cz = (int)floorf(from.z);
    for (int x = cx - r; x <= cx + r; x++) {
        if (x < 0 || x >= WDX) continue;
        for (int z = cz - r; z <= cz + r; z++) {
            if (z < 0 || z >= WDZ) continue;
            for (int y = 0; y < WDY; y++) {
                if (world.get(x, y, z) == BL_CROP) {
                    float d = (float)((x - from.x) * (x - from.x) + (y - from.y) * (y - from.y) + (z - from.z) * (z - from.z));
                    if (d < bestDist) {
                        bestDist = d;
                        best = { (float)x + 0.5f, (float)y, (float)z + 0.5f };
                    }
                }
            }
        }
    }
    return best;
}

void updateNPC(NPC& npc, VoxelWorld& world, std::vector<Kingdom>& kingdoms,
               std::vector<Monster>& monsters, float dt) {
    if (!npc.alive) return;
    npc.timer -= dt;
    if (npc.attackCooldown > 0) npc.attackCooldown -= dt;

    Kingdom& k = kingdoms[npc.kingdomId];

    int nearestMonsterId = -1;
    float nearestMonsterDist = 1e9f;
    for (size_t i = 0; i < monsters.size(); i++) {
        if (!monsters[i].alive) continue;
        float d = Vector3Distance(monsters[i].pos, npc.pos);
        if (d < nearestMonsterDist) { nearestMonsterDist = d; nearestMonsterId = (int)i; }
    }

    if (npc.type == NPC_FARMER) {
        if (nearestMonsterId >= 0 && nearestMonsterDist < 8.0f) npc.state = NS_FLEE;

        switch (npc.state) {
            case NS_IDLE: {
                if (npc.timer <= 0) {
                    Vector3 crop = findNearestCrop(world, npc.pos);
                    if (crop.x >= 0) { npc.target = crop; npc.state = NS_MOVE_TO_CROP; }
                    else npc.timer = 2.0f;
                }
                npc.vel.x = 0; npc.vel.z = 0;
                break;
            }
            case NS_MOVE_TO_CROP: {
                moveNPCOnGround(npc, npc.target, 2.5f, dt, world);
                Vector3 d = { npc.target.x - npc.pos.x, 0, npc.target.z - npc.pos.z };
                if (sqrtf(d.x * d.x + d.z * d.z) < 1.0f) {
                    npc.state = NS_HARVEST;
                    npc.timer = 1.5f;
                }
                break;
            }
            case NS_HARVEST: {
                if (npc.timer <= 0) {
                    int cx = (int)floorf(npc.target.x);
                    int cy = (int)floorf(npc.target.y);
                    int cz = (int)floorf(npc.target.z);
                    if (world.get(cx, cy, cz) == BL_CROP) {
                        world.set(cx, cy, cz, BL_AIR);
                        npc.carriedFood += 3;
                    }
                    npc.state = NS_RETURN_HOME;
                    npc.target = k.center;
                }
                break;
            }
            case NS_RETURN_HOME: {
                moveNPCOnGround(npc, k.center, 2.5f, dt, world);
                Vector3 d = { k.center.x - npc.pos.x, 0, k.center.z - npc.pos.z };
                if (sqrtf(d.x * d.x + d.z * d.z) < 2.0f) {
                    npc.state = NS_DEPOSIT;
                    npc.timer = 0.5f;
                }
                break;
            }
            case NS_DEPOSIT: {
                if (npc.timer <= 0) {
                    k.food += npc.carriedFood;
                    npc.carriedFood = 0;
                    npc.state = NS_IDLE;
                    npc.timer = 1.0f;
                }
                break;
            }
            case NS_FLEE: {
                if (nearestMonsterId >= 0 && nearestMonsterDist < 12.0f) {
                    Vector3 fleeDir = { npc.pos.x - monsters[nearestMonsterId].pos.x, 0,
                                        npc.pos.z - monsters[nearestMonsterId].pos.z };
                    float len = sqrtf(fleeDir.x * fleeDir.x + fleeDir.z * fleeDir.z);
                    if (len > 0.001f) {
                        fleeDir.x /= len; fleeDir.z /= len;
                        Vector3 t = { npc.pos.x + fleeDir.x * 5, npc.pos.y, npc.pos.z + fleeDir.z * 5 };
                        moveNPCOnGround(npc, t, 4.5f, dt, world);
                    }
                } else {
                    npc.state = NS_IDLE;
                    npc.timer = 0.5f;
                }
                break;
            }
            default: npc.state = NS_IDLE; npc.timer = 0; break;
        }
    } else if (npc.type == NPC_GUARD) {
        if (nearestMonsterId >= 0 && nearestMonsterDist < 15.0f) {
            npc.state = NS_ATTACK;
            npc.targetMonsterId = nearestMonsterId;
        } else if (npc.state == NS_ATTACK) {
            npc.state = NS_PATROL;
            npc.timer = 1.0f;
        }

        switch (npc.state) {
            case NS_IDLE:
            case NS_PATROL: {
                if (npc.timer <= 0) {
                    float ang = GetRandomValue(0, 360) * DEG2RAD;
                    float r = (float)GetRandomValue(2, (int)k.radius);
                    npc.target = { k.center.x + cosf(ang) * r, k.center.y, k.center.z + sinf(ang) * r };
                    npc.timer = 3.0f;
                    npc.state = NS_PATROL;
                }
                moveNPCOnGround(npc, npc.target, 3.0f, dt, world);
                Vector3 d = { npc.target.x - npc.pos.x, 0, npc.target.z - npc.pos.z };
                if (sqrtf(d.x * d.x + d.z * d.z) < 1.5f) npc.timer = 0;
                break;
            }
            case NS_ATTACK: {
                if (npc.targetMonsterId < 0 || npc.targetMonsterId >= (int)monsters.size()
                    || !monsters[npc.targetMonsterId].alive) {
                    npc.state = NS_PATROL;
                    npc.timer = 1.0f;
                    break;
                }
                Monster& m = monsters[npc.targetMonsterId];
                moveNPCOnGround(npc, m.pos, 4.5f, dt, world);
                if (Vector3Distance(m.pos, npc.pos) < 1.5f && npc.attackCooldown <= 0) {
                    m.health -= 25.0f;
                    npc.attackCooldown = 1.0f;
                    if (m.health <= 0) {
                        m.alive = false;
                        k.reputation = std::min(100, k.reputation + 2);
                    }
                }
                break;
            }
            default: npc.state = NS_PATROL; npc.timer = 0; break;
        }
    }
}

void updateMonster(Monster& m, Player& player, std::vector<NPC>& npcs, float dt, const VoxelWorld& world) {
    if (!m.alive) return;
    if (m.attackCooldown > 0) m.attackCooldown -= dt;

    int targetNpc = -1;
    float nearestDist = 1e9f;
    for (size_t i = 0; i < npcs.size(); i++) {
        if (!npcs[i].alive) continue;
        float d = Vector3Distance(npcs[i].pos, m.pos);
        if (d < nearestDist) { nearestDist = d; targetNpc = (int)i; }
    }
    float playerDist = Vector3Distance(player.pos, m.pos);

    Vector3 target;
    bool targetIsPlayer = false;
    if (playerDist < nearestDist && playerDist < 20.0f) {
        target = player.pos;
        targetIsPlayer = true;
    } else if (targetNpc >= 0) {
        target = npcs[targetNpc].pos;
    } else {
        target = player.pos;
        targetIsPlayer = true;
    }

    Vector3 toTarget = { target.x - m.pos.x, 0, target.z - m.pos.z };
    float dist = sqrtf(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
    if (dist > 0.1f) {
        m.vel.x = (toTarget.x / dist) * 3.5f;
        m.vel.z = (toTarget.z / dist) * 3.5f;
    }

    Vector3 newPos = m.pos;
    newPos.x += m.vel.x * dt;
    newPos.z += m.vel.z * dt;

    int by = (int)floorf(m.pos.y);
    int bx = (int)floorf(newPos.x);
    int bz = (int)floorf(newPos.z);
    bool blocked = world.isSolidAt(bx, by, bz) || world.isSolidAt(bx, by + 1, bz);

    if (!blocked) {
        m.pos.x = newPos.x;
        m.pos.z = newPos.z;
    } else {
        if (!world.isSolidAt(bx, by + 1, bz) && !world.isSolidAt(bx, by + 2, bz)) {
            m.pos.y += 1.0f;
            m.pos.x = newPos.x;
            m.pos.z = newPos.z;
        } else {
            int cx = (int)floorf(newPos.x);
            int cz = (int)floorf(m.pos.z);
            if (!world.isSolidAt(cx, by, cz) && !world.isSolidAt(cx, by + 1, cz)) m.pos.x = newPos.x;
            int cx2 = (int)floorf(m.pos.x);
            int cz2 = (int)floorf(newPos.z);
            if (!world.isSolidAt(cx2, by, cz2) && !world.isSolidAt(cx2, by + 1, cz2)) m.pos.z = newPos.z;
        }
    }

    float groundY = findGroundY(world, m.pos);
    if (m.pos.y < groundY + 0.4f) m.pos.y = groundY + 0.4f;
    else if (m.pos.y > groundY + 0.5f) {
        m.pos.y -= 15.0f * dt;
        if (m.pos.y < groundY + 0.4f) m.pos.y = groundY + 0.4f;
    } else {
        m.pos.y = groundY + 0.4f;
    }

    float attackRange = 1.5f;
    if (targetIsPlayer) {
        if (playerDist < attackRange && m.attackCooldown <= 0) {
            player.health -= 10.0f;
            m.attackCooldown = 1.5f;
        }
    } else if (targetNpc >= 0) {
        if (Vector3Distance(npcs[targetNpc].pos, m.pos) < attackRange && m.attackCooldown <= 0) {
            npcs[targetNpc].health -= 15.0f;
            m.attackCooldown = 1.5f;
            if (npcs[targetNpc].health <= 0) npcs[targetNpc].alive = false;
        }
    }

    if (GetTime() - m.spawnTime > 60.0f) m.alive = false;
}

// ============================================================
// Main
// ============================================================
int main() {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "Voxel Kingdom - 3D Sandbox Strategy");
    SetTargetFPS(60);
    DisableCursor();

    VoxelWorld world;
    world.generate();

    Player player;
    int spawnX = WDX / 2, spawnZ = WDZ / 2;
    int spawnY = world.surfaceHeight(spawnX, spawnZ) + 1;
    for (int y = spawnY; y < WDY; y++) world.set(spawnX, y, spawnZ, BL_AIR);
    player.pos = { (float)spawnX + 0.5f, (float)spawnY, (float)spawnZ + 0.5f };

    std::vector<Kingdom> kingdoms;
    kingdoms.push_back({ "Verdantia", 200, 50, 10,
        MKCOL(80, 180, 80), { 20.5f, 0, 20.5f }, 8.0f, 0.0f });
    kingdoms.push_back({ "Stonewall", 150, 30, -5,
        MKCOL(150, 150, 160), { 44.5f, 0, 44.5f }, 8.0f, 0.0f });

    std::vector<NPC> npcs;
    int npcIdCounter = 0;
    auto makeNPC = [&](int kingdomId, int type, float ox, float oz) {
        NPC n;
        n.id = npcIdCounter++;
        n.kingdomId = kingdomId;
        n.type = type;
        n.pos = { kingdoms[kingdomId].center.x + ox, 0, kingdoms[kingdomId].center.z + oz };
        n.pos.y = (float)world.surfaceHeight((int)n.pos.x, (int)n.pos.z) + 1;
        n.vel = { 0, 0, 0 };
        n.state = (type == NPC_FARMER) ? NS_IDLE : NS_PATROL;
        n.timer = (float)GetRandomValue(0, 100) / 100.0f;
        n.health = (type == NPC_FARMER) ? 60.0f : 100.0f;
        n.maxHealth = n.health;
        n.carriedFood = 0;
        n.targetMonsterId = -1;
        n.attackCooldown = 0;
        n.alive = true;
        npcs.push_back(n);
    };
    makeNPC(0, NPC_FARMER, -3, 0);
    makeNPC(0, NPC_FARMER,  3, 0);
    makeNPC(0, NPC_GUARD,   0, 3);
    makeNPC(1, NPC_FARMER, -3, 0);
    makeNPC(1, NPC_GUARD,   3, 0);
    makeNPC(1, NPC_GUARD,   0, -3);

    std::vector<Monster> monsters;
    int monsterIdCounter = 0;
    float monsterSpawnTimer = 5.0f;

    float dayTime = 0.25f;

    Camera3D camera = { 0 };
    camera.up = { 0, 1, 0 };
    camera.fovy = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    bool showDebug = false;
    bool showHelp = true;
    float helpTimer = 8.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        dayTime += dt / DAY_LENGTH;
        if (dayTime >= 1.0f) dayTime -= 1.0f;
        bool isNight = (dayTime > 0.5f && dayTime < 1.0f);
        if (helpTimer > 0) helpTimer -= dt;

        if (IsKeyPressed(KEY_E)) {
            player.inShop = !player.inShop;
            if (player.inShop) EnableCursor(); else DisableCursor();
        }
        if (IsKeyPressed(KEY_F1)) showHelp = !showHelp;
        if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;

        if (!player.inShop) {
            updatePlayer(player, world, dt);

            for (int i = 0; i < HOTBAR_SIZE; i++) {
                if (IsKeyPressed(KEY_ONE + i)) player.selectedBlock = i;
            }
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                player.selectedBlock = (player.selectedBlock + (wheel > 0 ? -1 : 1) + HOTBAR_SIZE) % HOTBAR_SIZE;
            }
            if (IsKeyPressed(KEY_Q)) {
                if (player.hasSword) player.swordEquipped = !player.swordEquipped;
            }

            int hitX, hitY, hitZ, faceX, faceY, faceZ;
            Vector3 eye = getPlayerEye(player);
            Vector3 fwd = getPlayerForward(player);
            bool hit = raycastVoxel(world, eye, fwd, REACH, hitX, hitY, hitZ, faceX, faceY, faceZ);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (player.swordEquipped) {
                    for (auto& m : monsters) {
                        if (!m.alive) continue;
                        Vector3 toM = { m.pos.x - eye.x, m.pos.y + 0.5f - eye.y, m.pos.z - eye.z };
                        float d = Vector3Length(toM);
                        if (d < REACH && d > 0.1f) {
                            toM = Vector3Scale(toM, 1.0f / d);
                            if (Vector3DotProduct(toM, fwd) > 0.7f && player.attackCooldown <= 0) {
                                m.health -= 35.0f;
                                player.attackCooldown = 0.4f;
                                if (m.health <= 0) {
                                    m.alive = false;
                                    player.gold += 15;
                                    int nearestK = 0;
                                    float nearestKDist = 1e9f;
                                    for (size_t kk = 0; kk < kingdoms.size(); kk++) {
                                        float d2 = Vector3Distance(kingdoms[kk].center, player.pos);
                                        if (d2 < nearestKDist) { nearestKDist = d2; nearestK = (int)kk; }
                                    }
                                    kingdoms[nearestK].reputation = std::min(100, kingdoms[nearestK].reputation + 5);
                                }
                                break;
                            }
                        }
                    }
                } else {
                    if (hit) {
                        BlockType b = world.get(hitX, hitY, hitZ);
                        if (b == BL_CROP) player.food += 1;
                        else if (b == BL_WOOD) player.wood += 1;
                        else if (b == BL_STONE) player.stone += 1;
                        world.set(hitX, hitY, hitZ, BL_AIR);
                    }
                }
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && hit) {
                int px = hitX + faceX, py = hitY + faceY, pz = hitZ + faceZ;
                BlockType toPlace = HOTBAR[player.selectedBlock];
                bool canPlace = true;
                if (toPlace == BL_WOOD && player.wood <= 0) canPlace = false;
                else if (toPlace == BL_STONE && player.stone <= 0) canPlace = false;
                else if (toPlace == BL_CROP && player.food <= 0) canPlace = false;
                else if (toPlace == BL_PLANK && player.wood <= 0) canPlace = false;

                if (canPlace && world.get(px, py, pz) == BL_AIR) {
                    Vector3 pmin = { player.pos.x - 0.3f, player.pos.y, player.pos.z - 0.3f };
                    Vector3 pmax = { player.pos.x + 0.3f, player.pos.y + PLAYER_HEIGHT, player.pos.z + 0.3f };
                    if ((float)(px+1) > pmin.x && (float)px < pmax.x &&
                        (float)(py+1) > pmin.y && (float)py < pmax.y &&
                        (float)(pz+1) > pmin.z && (float)pz < pmax.z) {
                        canPlace = false;
                    }
                }
                if (canPlace && world.get(px, py, pz) == BL_AIR) {
                    world.set(px, py, pz, toPlace);
                    if (toPlace == BL_WOOD || toPlace == BL_PLANK) player.wood--;
                    else if (toPlace == BL_STONE) player.stone--;
                    else if (toPlace == BL_CROP) player.food--;
                }
            }

            if (IsKeyPressed(KEY_F) && player.food > 0 && player.health < player.maxHealth) {
                player.food--;
                player.health = std::min(player.maxHealth, player.health + 20.0f);
            }
        }

        for (auto& npc : npcs) updateNPC(npc, world, kingdoms, monsters, dt);
        for (auto& m : monsters) updateMonster(m, player, npcs, dt, world);
        monsters.erase(std::remove_if(monsters.begin(), monsters.end(),
            [](const Monster& m) { return !m.alive; }), monsters.end());

        if (isNight) {
            monsterSpawnTimer -= dt;
            if (monsterSpawnTimer <= 0 && (int)monsters.size() < 5) {
                Monster m;
                m.id = monsterIdCounter++;
                int sx, sz, tries = 0;
                do {
                    sx = GetRandomValue(2, WDX - 3);
                    sz = GetRandomValue(2, WDZ - 3);
                    tries++;
                } while (Vector3Distance({ (float)sx + 0.5f, 0, (float)sz + 0.5f }, player.pos) < 10.0f && tries < 20);
                int sy = world.surfaceHeight(sx, sz);
                m.pos = { (float)sx + 0.5f, (float)sy + 0.4f, (float)sz + 0.5f };
                m.vel = { 0, 0, 0 };
                m.health = 70.0f;
                m.attackCooldown = 0;
                m.alive = true;
                m.spawnTime = (float)GetTime();
                monsters.push_back(m);
                monsterSpawnTimer = 8.0f;
            }
        } else {
            monsterSpawnTimer = 3.0f;
        }

        for (auto& k : kingdoms) {
            k.foodTimer -= dt;
            if (k.foodTimer <= 0) {
                k.foodTimer = 5.0f;
                int cropCount = 0, wateredCrops = 0;
                int x0 = (int)(k.center.x - k.radius - 2), x1 = (int)(k.center.x + k.radius + 2);
                int z0 = (int)(k.center.z - k.radius - 2), z1 = (int)(k.center.z + k.radius + 2);
                for (int x = x0; x <= x1; x++)
                    for (int z = z0; z <= z1; z++)
                        for (int y = 0; y < WDY; y++)
                            if (world.get(x, y, z) == BL_CROP) {
                                cropCount++;
                                if (world.hasWaterNeighbor(x, y, z)) wateredCrops++;
                            }
                k.food += cropCount + wateredCrops * 2;
                if (k.food > 50) { k.gold += 1; k.food -= 2; }
                if (GetRandomValue(0, 100) < 30) {
                    int cx = (int)(k.center.x + GetRandomValue(-7, 7));
                    int cz = (int)(k.center.z + GetRandomValue(-7, 7));
                    int sy = world.surfaceHeight(cx, cz);
                    if (world.get(cx, sy, cz) == BL_GRASS && world.get(cx, sy + 1, cz) == BL_AIR) {
                        world.set(cx, sy + 1, cz, BL_CROP);
                    }
                }
            }
        }

        if (player.health <= 0) {
            player.health = player.maxHealth;
            player.pos = { (float)spawnX + 0.5f, (float)spawnY, (float)spawnZ + 0.5f };
            player.vel = { 0, 0, 0 };
            player.gold = std::max(0, player.gold / 2);
        }

        Vector3 eye = getPlayerEye(player);
        Vector3 fwd = getPlayerForward(player);
        camera.position = eye;
        camera.target = Vector3Add(eye, fwd);

        Color skyColor;
        if (dayTime < 0.45f)      skyColor = MKCOL(135, 206, 235);
        else if (dayTime < 0.5f)  {
            float t = (dayTime - 0.45f) / 0.05f;
            skyColor = MKCOL((int)(135 + 120 * t), (int)(206 - 106 * t), (int)(235 - 185 * t));
        } else if (dayTime < 0.55f) {
            float t = (dayTime - 0.5f) / 0.05f;
            skyColor = MKCOL((int)(255 - 235 * t), (int)(100 - 70 * t), (int)(50 + 10 * t));
        } else if (dayTime < 0.95f) skyColor = MKCOL(20, 30, 60);
        else {
            float t = (dayTime - 0.95f) / 0.05f;
            skyColor = MKCOL((int)(20 + 115 * t), (int)(30 + 176 * t), (int)(60 + 175 * t));
        }

        // ===================== DRAW =====================
        BeginDrawing();
        ClearBackground(skyColor);

        BeginMode3D(camera);

        world.draw();

        // Block highlight
        {
            int hX, hY, hZ, fX, fY, fZ;
            Vector3 eye2 = getPlayerEye(player);
            Vector3 fwd2 = getPlayerForward(player);
            if (raycastVoxel(world, eye2, fwd2, REACH, hX, hY, hZ, fX, fY, fZ)) {
                Vector3 bp = { (float)hX + 0.5f, (float)hY + 0.5f, (float)hZ + 0.5f };
                drawCubeWiresCustom(bp, 1.02f, 1.02f, 1.02f, BLACK);
            }
        }

        // Draw NPCs
        for (const auto& npc : npcs) {
            if (!npc.alive) continue;
            Color c = kingdoms[npc.kingdomId].color;
            if (npc.type == NPC_FARMER) {
                DrawCylinder({ npc.pos.x, npc.pos.y + 0.5f, npc.pos.z }, 0.3f, 0.3f, 1.0f, 8, c);
                DrawSphere({ npc.pos.x, npc.pos.y + 1.1f, npc.pos.z }, 0.25f, MKCOL(220, 180, 140));
                DrawCylinder({ npc.pos.x, npc.pos.y + 1.35f, npc.pos.z }, 0.05f, 0.25f, 0.1f, 8, MKCOL(180, 140, 60));
            } else {
                DrawCylinder({ npc.pos.x, npc.pos.y + 0.6f, npc.pos.z }, 0.35f, 0.35f, 1.2f, 8, c);
                DrawSphere({ npc.pos.x, npc.pos.y + 1.3f, npc.pos.z }, 0.28f, MKCOL(220, 180, 140));
                DrawSphere({ npc.pos.x, npc.pos.y + 1.4f, npc.pos.z }, 0.32f, MKCOL(80, 80, 90));
                DrawCube({ npc.pos.x + 0.4f, npc.pos.y + 0.7f, npc.pos.z }, 0.05f, 0.6f, 0.05f, GRAY);
            }
            float hpFrac = npc.health / npc.maxHealth;
            float hbY = npc.pos.y + (npc.type == NPC_FARMER ? 1.7f : 1.9f);
            DrawLine3D({ npc.pos.x - 0.4f, hbY, npc.pos.z }, { npc.pos.x + 0.4f, hbY, npc.pos.z }, RED);
            DrawLine3D({ npc.pos.x - 0.4f, hbY, npc.pos.z }, { npc.pos.x - 0.4f + 0.8f * hpFrac, hbY, npc.pos.z }, GREEN);
        }

        // Draw Monsters
        for (const auto& m : monsters) {
            if (!m.alive) continue;
            DrawCube(m.pos, 0.8f, 0.8f, 0.8f, RED);
            drawCubeWiresCustom(m.pos, 0.8f, 0.8f, 0.8f, MAROON);
            DrawSphere({ m.pos.x, m.pos.y + 0.5f, m.pos.z }, 0.2f, BLACK);
            
            float hbY = m.pos.y + 0.8f;
            DrawLine3D({ m.pos.x - 0.4f, hbY, m.pos.z }, { m.pos.x + 0.4f, hbY, m.pos.z }, RED);
            DrawLine3D({ m.pos.x - 0.4f, hbY, m.pos.z }, { m.pos.x - 0.4f + 0.8f * (m.health / 70.0f), hbY, m.pos.z }, GREEN);
        }

        // Draw Kingdom Totems
        for (const auto& k : kingdoms) {
            int sy = world.surfaceHeight((int)k.center.x, (int)k.center.z);
            Vector3 base = { k.center.x, (float)sy + 1.5f, k.center.z };
            DrawCylinder(base, 0.2f, 0.2f, 3.0f, 8, k.color);
            DrawCube({ k.center.x, (float)sy + 3.0f, k.center.z }, 1.5f, 1.0f, 0.1f, k.color);
        }

        EndMode3D();

        // ===================== UI =====================
        DrawText(TextFormat("HP: %d/%d", (int)player.health, (int)player.maxHealth), 20, 20, 20, RED);
        DrawText(TextFormat("Gold: %d", player.gold), 20, 50, 20, YELLOW);
        DrawText(TextFormat("Wood: %d  Stone: %d  Food: %d", player.wood, player.stone, player.food), 20, 80, 20, BROWN);
        DrawText(TextFormat("Time: %s", isNight ? "Night" : "Day"), 20, 110, 20, RAYWHITE);
        
        if (player.swordEquipped) DrawText("Sword Equipped", 20, 140, 20, WHITE);
        else {
            BlockType selBlock = HOTBAR[player.selectedBlock];
            DrawText(TextFormat("Holding: %s", blockName(selBlock)), 20, 140, 20, WHITE);
        }

        // Hotbar
        for(int i = 0; i < HOTBAR_SIZE; i++) {
            int x = 20 + i * 35;
            int y = GetScreenHeight() - 50;
            Color c = blockColor(HOTBAR[i]);
            DrawRectangle(x, y, 30, 30, c);
            if (i == player.selectedBlock) {
                DrawRectangleLines(x-2, y-2, 34, 34, WHITE);
            }
            DrawText(TextFormat("%d", i+1), x, y - 15, 15, WHITE);
        }

        // Crosshair
        DrawLine(GetScreenWidth()/2 - 10, GetScreenHeight()/2, GetScreenWidth()/2 + 10, GetScreenHeight()/2, WHITE);
        DrawLine(GetScreenWidth()/2, GetScreenHeight()/2 - 10, GetScreenWidth()/2, GetScreenHeight()/2 + 10, WHITE);

        // Kingdom statuses
        int ky = 20;
        for (const auto& k : kingdoms) {
            DrawText(TextFormat("%s - G:%d F:%d Rep:%d", k.name.c_str(), k.gold, k.food, k.reputation), GetScreenWidth() - 300, ky, 20, k.color);
            ky += 25;
        }

        // Shop UI
        if (player.inShop) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));
            int cx = GetScreenWidth() / 2;
            int cy = GetScreenHeight() / 2;
            DrawRectangle(cx - 200, cy - 180, 400, 360, Fade(DARKGRAY, 0.9f));
            DrawRectangleLines(cx - 200, cy - 180, 400, 360, YELLOW);
            DrawText("KINGDOM SHOP (Press E to close)", cx - 180, cy - 160, 20, YELLOW);

            // Simulated local economy: prices vary depending on available kingdom resources
            int woodPrice = 5 + kingdoms[0].food / 10;
            int stonePrice = 8 + kingdoms[1].gold / 20;
            int swordPrice = 50 + kingdoms[1].reputation;
            int foodPrice = 10 + kingdoms[0].reputation / 10;

            DrawText(TextFormat("1. Buy Wood x5  (%d Gold)", woodPrice), cx - 180, cy - 100, 20, WHITE);
            DrawText(TextFormat("2. Buy Stone x5 (%d Gold)", stonePrice), cx - 180, cy - 70, 20, WHITE);
            DrawText(TextFormat("3. Buy Sword    (%d Gold)", swordPrice), cx - 180, cy - 40, 20, WHITE);
            DrawText(TextFormat("4. Sell Food x5 (+%d Gold)", foodPrice), cx - 180, cy - 10, 20, WHITE);
            DrawText(TextFormat("Your Gold: %d", player.gold), cx - 180, cy + 30, 20, YELLOW);

            if (IsKeyPressed(KEY_ONE)) {
                if (player.gold >= woodPrice) { player.gold -= woodPrice; player.wood += 5; }
            }
            if (IsKeyPressed(KEY_TWO)) {
                if (player.gold >= stonePrice) { player.gold -= stonePrice; player.stone += 5; }
            }
            if (IsKeyPressed(KEY_THREE)) {
                if (player.gold >= swordPrice && !player.hasSword) { 
                    player.gold -= swordPrice; 
                    player.hasSword = true; 
                    player.swordEquipped = true; 
                }
            }
            if (IsKeyPressed(KEY_FOUR)) {
                if (player.food >= 5) { 
                    player.food -= 5; 
                    player.gold += foodPrice; 
                    kingdoms[0].food += 5; 
                    kingdoms[0].gold -= foodPrice; 
                    if(kingdoms[0].gold < 0) kingdoms[0].gold = 0; 
                }
            }
        } else {
            if (helpTimer > 0 || showHelp) {
                DrawText("WASD: Move, SPACE: Jump, L-Click: Mine/Attack, R-Click: Place", 20, GetScreenHeight() - 100, 18, RAYWHITE);
                DrawText("1-8/Scroll: Select Block, Q: Toggle Sword, E: Shop, F: Eat", 20, GetScreenHeight() - 80, 18, RAYWHITE);
                DrawText("F1: Toggle Help, F3: Debug Info", 20, GetScreenHeight() - 60, 18, RAYWHITE);
            }
            if (showDebug) {
                DrawText(TextFormat("FPS: %d", GetFPS()), GetScreenWidth() - 100, GetScreenHeight() - 40, 20, GREEN);
                DrawText(TextFormat("Pos: %.1f, %.1f, %.1f", player.pos.x, player.pos.y, player.pos.z), GetScreenWidth() - 250, GetScreenHeight() - 20, 20, GREEN);
                DrawText(TextFormat("Monsters: %d", (int)monsters.size()), GetScreenWidth() - 250, GetScreenHeight() - 40, 20, RED);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}