/* -*- C++ -*-
* File: 4channels.cpp
* Copyright 2008-2017 LibRaw LLC (info@libraw.org)
* Created: Mon Feb 09, 2009
*
* LibRaw sample
* Generates 4 TIFF file from RAW data, one file per channel
*

LibRaw is free software; you can redistribute it and/or modify
it under the terms of the one of two licenses as you choose:

1. GNU LESSER GENERAL PUBLIC LICENSE version 2.1
(See file LICENSE.LGPL provided in LibRaw distribution archive for details).

2. COMMON DEVELOPMENT AND DISTRIBUTION LICENSE (CDDL) Version 1.0
(See file LICENSE.CDDL provided in LibRaw distribution archive for details).


*/

#include "demosaic.h"
#include "simpletiff.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#ifndef WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

#include <libraw.h>
#include <cassert>

#ifdef WIN32
#define snprintf _snprintf
#endif

int main(int ac, char *av[])
{
    int  i, ret;
    int autoscale = 0, black_subtraction = 1, use_gamma = 0;
    char outfn[1024];

    LibRaw RawProcessor;
    if (ac<2)
    {
    usage:
        printf(
            "4channels - LibRaw %s sample. %d cameras supported\n"
            "Usage: %s [-s N] [-g] [-A] [-B] [-N] raw-files....\n"
            "\t-s N - select Nth image in file (default=0)\n"
            "\t-g - use gamma correction with gamma 2.2 (not precise,use for visual inspection only)\n"
            "\t-A - autoscaling (by integer factor)\n"
            "\t-B - no black subtraction\n"
            , LibRaw::version(),
            LibRaw::cameraCount(),
            av[0]);
        return 0;
    }

    auto& P1 = RawProcessor.imgdata.idata;
    auto& sizes = RawProcessor.imgdata.sizes;
    auto& color = RawProcessor.imgdata.color;
    auto& thumbnail = RawProcessor.imgdata.thumbnail;
    auto& P2 = RawProcessor.imgdata.other;
    auto& out = RawProcessor.imgdata.params;

    out.output_bps = 16;
    out.output_tiff = 1;
    out.user_flip = 0;
    out.no_auto_bright = 1;
    out.half_size = 1;

    for (i = 1; i<ac; i++)
    {
        if (av[i][0] == '-')
        {
            if (av[i][1] == 's' && av[i][2] == 0)
            {
                i++;
                out.shot_select = av[i] ? atoi(av[i]) : 0;
            }
            else if (av[i][1] == 'g' && av[i][2] == 0)
                use_gamma = 1;
            else if (av[i][1] == 'A' && av[i][2] == 0)
                autoscale = 1;
            else if (av[i][1] == 'B' && av[i][2] == 0)
            {
                black_subtraction = 0;
            }
            else
                goto usage;
            continue;
        }
        if (!use_gamma)
            out.gamm[0] = out.gamm[1] = 1;

        int c;
        printf("Processing file %s\n", av[i]);
        if ((ret = RawProcessor.open_file(av[i])) != LIBRAW_SUCCESS)
        {
            fprintf(stderr, "Cannot open %s: %s\n", av[i], libraw_strerror(ret));
            continue; // no recycle b/c open file will recycle itself
        }
        if (P1.is_foveon)
        {
            printf("Cannot process Foveon image %s\n", av[i]);
            continue;
        }
        if ((ret = RawProcessor.unpack()) != LIBRAW_SUCCESS)
        {
            fprintf(stderr, "Cannot unpack %s: %s\n", av[i], libraw_strerror(ret));
            continue;
        }


        uint16_t width = RawProcessor.imgdata.rawdata.sizes.raw_width;
        uint16_t height = RawProcessor.imgdata.rawdata.sizes.raw_height;

        halide_buffer_t input = {};
        input.host = (uint8_t*)RawProcessor.imgdata.rawdata.raw_image;
        input.type = halide_type_t(halide_type_uint, 16);
        input.dimensions = 2;
        halide_dimension_t dims[2] = {};
        dims[1].min = 1;
        dims[0].extent = width;
        dims[1].extent = height - 2;
        dims[0].stride = 1;
        dims[1].stride = width;
        input.dim = dims;

        std::unique_ptr<int16_t[]> outputBuf(new int16_t[width*height*3]);
        halide_buffer_t output = {};
        output.host = (uint8_t*)outputBuf.get();
        output.type = halide_type_t(halide_type_int, 16);
        output.dimensions = 3;
        halide_dimension_t odims[3] = {};
        odims[0].extent = width;
        odims[1].extent = height;
        odims[2].extent = 3;
        odims[0].stride = 3;
        odims[1].stride = width;
        odims[2].stride = 1;
        output.dim = odims;

        int res = render(&input, &output);
        assert(!res);
        // @mem(input.host, UINT16, 1, width, height, width * 2)
        // @mem(output.host, INT16, 3, width, height, width * 2)

        int min = 0, max = 0;
        for (uint32_t i = 0; i < width*height * 3u; ++i)
        {
            if (outputBuf[i] < min)
                min = outputBuf[i];
            if (outputBuf[i] > max)
                max = outputBuf[i];
            outputBuf[i] += 150;
        }
        printf("min %d max %d\n", min, max);
        //lodepng_encode_file("out.png", (uint8_t*)outputBuf.get(), width, height, LCT_RGB, 16);
		toTIFF("out.tif", outputBuf.get(), width, height, 3);

        return 0;

        RawProcessor.raw2image();
        if (black_subtraction)
        {
            RawProcessor.subtract_black();
        }

        // @mem(RawProcessor.imgdata.rawdata.raw_image, UINT16, 1, RawProcessor.imgdata.rawdata.sizes.raw_width, RawProcessor.imgdata.rawdata.sizes.raw_height, RawProcessor.imgdata.rawdata.sizes.raw_width * 2)
        // @mem(RawProcessor.imgdata.image, UINT16, 1, RawProcessor.imgdata.sizes.width, RawProcessor.imgdata.sizes.height, RawProcessor.imgdata.sizes.width * 2)
        if (autoscale)
        {
            unsigned max = 0, scale = 1;
            for (int j = 0; j<sizes.iheight*sizes.iwidth; j++)
                for (int c = 0; c< 4; c++)
                    if (max < RawProcessor.imgdata.image[j][c])
                        max = RawProcessor.imgdata.image[j][c];
            if (max >0 && max< 1 << 15)
            {
                scale = (1 << 16) / max;
                printf("Scaling with multiplier=%d (max=%d)\n", scale, max);
                for (int j = 0; j<sizes.iheight*sizes.iwidth; j++)
                    for (c = 0; c<4; c++)
                        RawProcessor.imgdata.image[j][c] *= scale;
            }
            printf("Black level (scaled)=%d\n", color.black*scale);
        }
        else
            printf("Black level (unscaled)=%d\n", color.black);


        // hack to make dcraw tiff writer happy
        int isrgb = (P1.colors == 4 ? 0 : 1);
        P1.colors = 1;
        sizes.width = sizes.iwidth;
        sizes.height = sizes.iheight;

        for (int layer = 0; layer<4; layer++)
        {
            if (layer>0)
            {
                for (int rc = 0; rc < sizes.iheight*sizes.iwidth; rc++)
                    RawProcessor.imgdata.image[rc][0] = RawProcessor.imgdata.image[rc][layer];
            }
            char lname[8];
            if (isrgb)
            {
                snprintf(lname, 7, "%c", ((char*)("RGBG"))[layer]);
                if (layer == 3)
                    strcat(lname, "2");
            }
            else
                snprintf(lname, 7, "%c", ((char*)("GCMY"))[layer]);

            if (out.shot_select)
                snprintf(outfn, sizeof(outfn), "%s-%d.%s.tiff", av[i], out.shot_select, lname);
            else
                snprintf(outfn, sizeof(outfn), "%s.%s.tiff", av[i], lname);

            printf("Writing file %s\n", outfn);
            if (LIBRAW_SUCCESS != (ret = RawProcessor.dcraw_ppm_tiff_writer(outfn)))
                fprintf(stderr, "Cannot write %s: %s\n", outfn, libraw_strerror(ret));
        }

    }
    return 0;
}
