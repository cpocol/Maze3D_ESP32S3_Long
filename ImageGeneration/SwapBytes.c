#include <stdio.h>

extern const unsigned short Wolf128x128rot[];
extern const unsigned short WolfGRAY128x128rot[];

int main()
{
    {
    FILE* pf = fopen("..\\Texture_Wolf128x128rotSwappedBytes.h", "w");

    fprintf(pf, "#include <pgmspace.h>\n\n");
    fprintf(pf, "const unsigned short Wolf128x128rotSwappedBytes[0x4000] PROGMEM ={\n");

    for (int i = 0; i < 128 * 128; i++)
    {
        unsigned short color = Wolf128x128rot[i];
        color = (color % 256) * 256 + (color / 256);
        fprintf(pf, "0x%04X, ", color);
        if (i % 16 == 15)
            fprintf(pf, "\n");
    }

    fprintf(pf, "};\n");

    fclose(pf);
    }

    {
    FILE* pf = fopen("..\\Texture_WolfGRAY128x128rotSwappedBytes.h", "w");

    fprintf(pf, "#include <pgmspace.h>\n\n");
    fprintf(pf, "const unsigned short WolfGRAY128x128rotSwappedBytes[0x4000] PROGMEM ={\n");

    for (int i = 0; i < 128 * 128; i++)
    {
        unsigned short color = WolfGRAY128x128rot[i];
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
