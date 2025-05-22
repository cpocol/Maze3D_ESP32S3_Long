#include <stdio.h>

extern const unsigned short Wolf128x128rot[];
extern const unsigned short WolfGRAY128x128rot[];

typedef struct
{
    const unsigned short* TextureData;
    char TextureName[100];
} Texture;

Texture textures[] = {
    {Wolf128x128rot,     "Wolf128x128rot"},
    {WolfGRAY128x128rot, "WolfGRAY128x128rot"},
};

const int texturesNum = sizeof(textures)/sizeof(textures[0]);

int main()
{
    for (int t = 0; t < texturesNum; t++)
    {
        char dstFileName[100];
        sprintf(dstFileName, "..\\Texture_%s_SwappedBytes.h", textures[t].TextureName);
        FILE* pf = fopen(dstFileName, "w");

        fprintf(pf, "#include <pgmspace.h>\n\n");
        fprintf(pf, "const unsigned short %s_SwappedBytes[0x4000] PROGMEM ={\n", textures[t].TextureName);

        for (int i = 0; i < 128 * 128; i++)
        {
            unsigned short color = textures[t].TextureData[i];
            color = (color % 256) * 256 + (color / 256);
            fprintf(pf, "0x%04X, ", color);
            if (i % 16 == 15)
                fprintf(pf, "\n");
        }

        fprintf(pf, "};\n");

        fclose(pf);
    }

    return 0;
}
