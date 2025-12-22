#include <stdio.h>
#include <string.h>

#define PC
#define DOOM

#ifdef PC
    #define PROGMEM ""
    #define OUTPUT_FOLDER "..\\..\\Wolfenstein3D_PC_CPP\\Textures\\"
    #define PIXEL_DATA_TYPE "int"
#else //ESP32
    #define SWAP_BYTES
    #define PROGMEM " PROGMEM"
    #define OUTPUT_FOLDER "..\\include\\"
    #define PIXEL_DATA_TYPE "short"
#endif

extern const unsigned short Wolf128x128rot[];
extern const unsigned short WolfGRAY128x128rot[];
#ifdef DOOM
extern const unsigned short Doom000[];
extern const unsigned short Doom001[];
extern const unsigned short Doom002[];
extern const unsigned short Doom003[];
extern const unsigned short Doom004[];
extern const unsigned short Doom005[];
extern const unsigned short Doom006[];
extern const unsigned short Doom007[];
extern const unsigned short Doom008[];
extern const unsigned short Doom009[];

extern const unsigned short Doom010[];
extern const unsigned short Doom011[];
extern const unsigned short Doom012[];
extern const unsigned short Doom013[];
extern const unsigned short Doom014[];
extern const unsigned short Doom015[];
extern const unsigned short Doom016[];
extern const unsigned short Doom017[];
extern const unsigned short Doom018[];
extern const unsigned short Doom019[];

extern const unsigned short Doom020[];
extern const unsigned short Doom021[];
extern const unsigned short Doom022[];
extern const unsigned short Doom023[];
extern const unsigned short Doom024[];
extern const unsigned short Doom025[];
extern const unsigned short Doom026[];
extern const unsigned short Doom027[];
extern const unsigned short Doom028[];
extern const unsigned short Doom029[];

extern const unsigned short Doom030[];
extern const unsigned short Doom031[];
extern const unsigned short Doom032[];
extern const unsigned short Doom033[];
extern const unsigned short Doom034[];
extern const unsigned short Doom035[];
extern const unsigned short Doom036[];
extern const unsigned short Doom037[];
extern const unsigned short Doom038[];
extern const unsigned short Doom039[];

extern const unsigned short Doom040[];
extern const unsigned short Doom041[];
extern const unsigned short Doom042[];
extern const unsigned short Doom043[];
extern const unsigned short Doom044[];
extern const unsigned short Doom045[];
extern const unsigned short Doom046[];
extern const unsigned short Doom047[];
extern const unsigned short Doom048[];
extern const unsigned short Doom049[];

extern const unsigned short Doom050[];
extern const unsigned short Doom051[];
extern const unsigned short Doom052[];
extern const unsigned short Doom053[];
extern const unsigned short Doom054[];
extern const unsigned short Doom055[];
extern const unsigned short Doom056[];
extern const unsigned short Doom057[];
extern const unsigned short Doom058[];
extern const unsigned short Doom059[];

extern const unsigned short Doom060[];
extern const unsigned short Doom061[];
extern const unsigned short Doom062[];
extern const unsigned short Doom063[];
extern const unsigned short Doom064[];
extern const unsigned short Doom065[];
extern const unsigned short Doom066[];
extern const unsigned short Doom067[];
extern const unsigned short Doom068[];
extern const unsigned short Doom069[];
#endif

extern const unsigned short Sprite_PottedTree[];
extern const unsigned short Sprite_Barrel[];
extern const unsigned short Sprite_Lamp[];

typedef struct
{
    const unsigned short* TextureData;
    char TextureName[100];
} Texture;

Texture textures[] = {
    {Wolf128x128rot,     "Wolf128x128rot"},
    {WolfGRAY128x128rot, "WolfGRAY128x128rot"},
#ifdef DOOM
    {Doom001, "Doom000"},
    {Doom001, "Doom001"},
    {Doom002, "Doom002"},
    {Doom003, "Doom003"},
    {Doom004, "Doom004"},
    {Doom005, "Doom005"},
    {Doom006, "Doom006"},
    {Doom006, "Doom007"},
    {Doom008, "Doom008"},
    {Doom009, "Doom009"},

    {Doom010, "Doom010"},
    {Doom011, "Doom011"},
    {Doom012, "Doom012"},
    {Doom013, "Doom013"},
    {Doom014, "Doom014"},
    {Doom015, "Doom015"},
    {Doom016, "Doom016"},
    {Doom016, "Doom017"},
    {Doom018, "Doom018"},
    {Doom019, "Doom019"},

    {Doom020, "Doom020"},
    {Doom021, "Doom021"},
    {Doom022, "Doom022"},
    {Doom023, "Doom023"},
    {Doom024, "Doom024"},
    {Doom025, "Doom025"},
    {Doom026, "Doom026"},
    {Doom026, "Doom027"},
    {Doom028, "Doom028"},
    {Doom029, "Doom029"},

    {Doom030, "Doom030"},
    {Doom031, "Doom031"},
    {Doom032, "Doom032"},
    {Doom033, "Doom033"},
    {Doom034, "Doom034"},
    {Doom035, "Doom035"},
    {Doom036, "Doom036"},
    {Doom036, "Doom037"},
    {Doom038, "Doom038"},
    {Doom039, "Doom039"},

    {Doom040, "Doom040"},
    {Doom041, "Doom041"},
    {Doom042, "Doom042"},
    {Doom043, "Doom043"},
    {Doom044, "Doom044"},
    {Doom045, "Doom045"},
    {Doom046, "Doom046"},
    {Doom046, "Doom047"},
    {Doom048, "Doom048"},
    {Doom049, "Doom049"},

    {Doom050, "Doom050"},
    {Doom051, "Doom051"},
    {Doom052, "Doom052"},
    {Doom053, "Doom053"},
    {Doom054, "Doom054"},
    {Doom055, "Doom055"},
    {Doom056, "Doom056"},
    {Doom056, "Doom057"},
    {Doom058, "Doom058"},
    {Doom059, "Doom059"},

    {Doom060, "Doom060"},
    {Doom061, "Doom061"},
    {Doom062, "Doom062"},
    {Doom063, "Doom063"},
    {Doom064, "Doom064"},
    {Doom065, "Doom065"},
    {Doom066, "Doom066"},
    {Doom066, "Doom067"},
    {Doom068, "Doom068"},
    {Doom069, "Doom069"},
#endif
    {Sprite_PottedTree, "Sprite_PottedTree"},
    {Sprite_Barrel, "Sprite_Barrel"},
    {Sprite_Lamp, "Sprite_Lamp"},
};

const int texturesNum = sizeof(textures)/sizeof(textures[0]);

#define RGB(r, g, b)      ((unsigned int)(((unsigned char)(r) | ((unsigned short)((unsigned char)(g))<<8)) | (((unsigned int)(unsigned char)(b))<<16)))
#define GetRValue(rgb565) ((rgb565 >> 11) & 0x1F)
#define GetGValue(rgb565) ((rgb565 >> 5) & 0x3F)
#define GetBValue(rgb565)  (rgb565 & 0x1F)

int main()
{
#ifdef SWAP_BYTES
    char suffix[] = "_SwappedBytes";
#else
    char suffix[] = "";
#endif
    for (int t = 0; t < texturesNum; t++)
    {
        char dstFileName[100];
        sprintf(dstFileName, "%sTexture_%s%s.h", OUTPUT_FOLDER, textures[t].TextureName, suffix);
        printf("%s\n", dstFileName);
        FILE* pf = fopen(dstFileName, "w");

#ifndef PC
        fprintf(pf, "#include <pgmspace.h>\n\n");
#endif
        if (strcmp(textures[t].TextureName, "Wolf128x128rot") == 0)
            //gain 2 ms per frame by keeping texture(s) in RAM
            //unfortunatelly, having a second texture in RAM (as static variable) leads to crash
            //fprintf(pf, "unsigned short %s%s[0x4000] = {\n", textures[t].TextureName, suffix);
            fprintf(pf, "const unsigned %s %s%s[0x4000] = {\n",
                    PIXEL_DATA_TYPE, textures[t].TextureName, suffix);
        else
            fprintf(pf, "const unsigned %s %s%s[0x4000]%s = {\n",
                    PIXEL_DATA_TYPE, textures[t].TextureName, suffix, PROGMEM);

        for (int i = 0; i < 128 * 128; i++)
        {
#ifdef PC
            unsigned int color = textures[t].TextureData[i];
            color = RGB(GetBValue(color) * 8, GetGValue(color) * 4, GetRValue(color) * 8);
            fprintf(pf, "0x%06X, ", color);
#else
            unsigned short color = textures[t].TextureData[i];
#ifdef SWAP_BYTES
            color = (color % 256) * 256 + (color / 256);
#endif
            fprintf(pf, "0x%04X, ", color);
#endif
            if (i % 16 == 15)
                fprintf(pf, "\n");
        }

        fprintf(pf, "};\n");

        fclose(pf);
    }

    char texturesFileName[100];
    sprintf(texturesFileName, "%sTextures.h", OUTPUT_FOLDER);
    printf("%s\n", texturesFileName);
    FILE* pf = fopen(texturesFileName, "w");

    for (int t = 0; t < texturesNum; t++)
        fprintf(pf, "#include \"Texture_%s%s.h\"\n", textures[t].TextureName, suffix);

    fclose(pf);

    return 0;
}
