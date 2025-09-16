#define COMMON_WALL 11
#define DOOM_WALL 22
#define SPRITE 30
#define SPRITE_TREE (SPRITE + 0)
#define SPRITE_BARREL (SPRITE + 1)
#define WALK_THROUGH_SPRITE 40
#define SPRITE_LAMP (WALK_THROUGH_SPRITE + 0)

//short ones as well
#define CW COMMON_WALL
#define DW DOOM_WALL
#define ST SPRITE_TREE
#define SB SPRITE_BARREL
#define SL SPRITE_LAMP

const int8_t Map[16][16] = {
{11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11},
{11,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11},
{11,  0, SL,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, SB, 11},
{11,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, SL,  0,  0, 11},
{11,  0,  0, 11, 11,  0, 11, 11, 11, ST,  0,  0,  0,  0,  0, 11},
{11,  0,  0, 11,  0,  0,  0,  0,  0, ST,  0,  0,  0,  0,  0, 11},
{11,  0,  0, 11,  0,  0,  0,  0,  0, ST,  0,  0,  0,  0,  0, 11},
{11, SB,  0, 11,  0,  0,  0,  0,  0, ST,  0,  0,  0,  0,  0, 11},
{11,  0,  0, 11,  0,  0,  0,  0,  0, 11,  0,  0,  0,  0,  0, 11},
{11, 11, 11, 11, 11,  0, SL,  0,  0,  0,  0,  0,  0,  0,  0, 11},
{11,  0,  0,  0, 11,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11},
{11,  0,  0,  0, 11,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11},
{11,  0,  0,  0, 11,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11},
{11,  0,  0,  0, 11,  0,  0,  0,  0,  0,  0,  0,  0, SL,  0, 11},
{11,  0,  0,  0, 11,  0,  0,  0,  0,  0,  0,  0,  0,  0, SB, 11},
{11, 11, 11, 11, 11, 11, DW, 11, 11, 11, 11, 11, 11, 11, 11, 11},
};

const int16_t mapWidth = sizeof(Map[0])/sizeof(Map[0][0]);
const int16_t mapHeight = sizeof(Map)/sizeof(Map[0]);
