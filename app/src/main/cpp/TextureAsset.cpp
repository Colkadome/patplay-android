#include <android/imagedecoder.h>
#include "TextureAsset.h"
#include "AndroidOut.h"
#include "Utility.h"

std::shared_ptr<TextureAsset>
TextureAsset::loadAsset(AAssetManager *assetManager, const std::string &assetPath, int removeWhiteOrBlack) {
    // Get the image from asset manager
    auto pAndroidRobotPng = AAssetManager_open(
            assetManager,
            assetPath.c_str(),
            AASSET_MODE_BUFFER);

    // Make a decoder to turn it into a texture
    AImageDecoder *pAndroidDecoder = nullptr;
    auto result = AImageDecoder_createFromAAsset(pAndroidRobotPng, &pAndroidDecoder);
    if (result != ANDROID_IMAGE_DECODER_SUCCESS) {
        return std::shared_ptr<TextureAsset>(new TextureAsset(0));
    }

    // make sure we get 8 bits per channel out. RGBA order.
    AImageDecoder_setAndroidBitmapFormat(pAndroidDecoder, ANDROID_BITMAP_FORMAT_RGBA_8888);

    // Get the image header, to help set everything up
    const AImageDecoderHeaderInfo *pAndroidHeader = nullptr;
    pAndroidHeader = AImageDecoder_getHeaderInfo(pAndroidDecoder);

    // important metrics for sending to GL
    auto width = AImageDecoderHeaderInfo_getWidth(pAndroidHeader);
    auto height = AImageDecoderHeaderInfo_getHeight(pAndroidHeader);
    auto stride = AImageDecoder_getMinimumStride(pAndroidDecoder);

    // Get the bitmap data of the image
    auto upAndroidImageData = std::make_unique<std::vector<uint8_t>>(height * stride);
    auto size = upAndroidImageData->size();
    auto data = upAndroidImageData->data();
    auto decodeResult = AImageDecoder_decodeImage(
            pAndroidDecoder,
            data,
            stride,
            size);

    // cleanup helpers
    AImageDecoder_delete(pAndroidDecoder);
    AAsset_close(pAndroidRobotPng);

    // Check result.
    if (decodeResult != ANDROID_IMAGE_DECODER_SUCCESS) {
        return std::shared_ptr<TextureAsset>(new TextureAsset(0));
    }

    // Get an opengl texture
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Clamp to the edge, you'll get odd results alpha blending if you don't
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Make every white pixel transparent.
    // This is a pat play thing.
    if (removeWhiteOrBlack == 1) {
        for (auto i = 0; i < size; i += 4) {
            if (data[i] > 0xf0 && data[i + 1] > 0xf0 && data[i + 2] > 0xf0) {
                data[i + 3] = 0;
            }
        }
    } else if (removeWhiteOrBlack == 2) {
        // Meant just for the font files. Invert and remove white. Lol.
        for (auto i = 0; i < size; i += 4) {
            if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 0) {
                data[i] = 0xff;
                data[i + 1] = 0xff;
                data[i + 2] = 0xff;
            } else {
                data[i + 3] = 0;
            }
        }
    }

    // Load the texture into VRAM
    glTexImage2D(
            GL_TEXTURE_2D, // target
            0, // mip level
            GL_RGBA, // internal format, often advisable to use BGR
            width, // width of the texture
            height, // height of the texture
            0, // border (always 0)
            GL_RGBA, // format
            GL_UNSIGNED_BYTE, // type
            data // Data to upload
    );

    // generate mip levels. Not really needed for 2D, but good to do
    // glGenerateMipmap(GL_TEXTURE_2D);

    // Create a shared pointer so it can be cleaned up easily/automatically
    return std::shared_ptr<TextureAsset>(new TextureAsset(textureId));
}

TextureAsset::~TextureAsset() {
    // return texture resources
    glDeleteTextures(1, &textureID_);
    textureID_ = 0;
}