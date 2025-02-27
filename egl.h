#pragma once

/*
获取 EGL Display 对象：eglGetDisplay()
初始化与 EGLDisplay 之间的连接：eglInitialize()
获取 EGLConfig 对象：eglChooseConfig()
创建 EGLContext 实例：eglCreateContext()
创建 EGLSurface 实例：eglCreateWindowSurface()
连接 EGLContext 和 EGLSurface：eglMakeCurrent()
使用 OpenGL ES API 绘制图形：gl_*()
切换 front buffer 和 back buffer 送显：eglSwapBuffer()
断开并释放与 EGLSurface 关联的 EGLContext 对象：eglRelease()
删除 EGLSurface 对象
删除 EGLContext 对象
终止与 EGLDisplay 之间的连接
*/

EGLint eglGetError();
EGLDisplay eglDisplay(EGLNativeDisplayType nativeDisplay); // EGL_DEFAULT_DISPLAY
EGLBoolean eglInitialize(EGLDisplay display, // 创建步骤时返回的对象
    EGLint *majorVersion, // 返回 EGL 主版本号
    EGLint *minorVersion); // 返回 EGL 次版本号

/*
EGLint majorVersion;
EGLint minorVersion;
EGLDisplay display;
display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
if (display == EGL_NO_DISPLAY) {
    // Unable to open connection to local windowing system
}
if (!eglInitialize(display, &majorVersion, &minorVersion)) {
    // Unable to initialize EGL. Handle and recover
}
*/

EGLBoolean eglGetConfigs(EGLDisplay display, // 指定显示的连接
    EGLConfig *configs, // 指定 GLConfig 列表
    EGLint maxReturnConfigs, // 最多返回的 GLConfig 数
    EGLint *numConfigs); // 实际返回的 GLConfig 数

EGLBoolean eglGetConfigAttrib(EGLDisplay display, // 指定显示的连接
    EGLConfig config, // 指定要查询的 GLConfig
    EGLint attribute, // 返回特定属性
    EGLint *value); // 返回值

EGLBoolean eglChooseChofig(EGLDispay display, // 指定显示的连接
    const EGLint *attribList, // 指定 configs 匹配的属性列表，可以为 NULL
    EGLConfig *config,   // 调用成功，返会符合条件的 EGLConfig 列表
    EGLint maxReturnConfigs, // 最多返回的符合条件的 GLConfig 数
    ELGint *numConfigs );  // 实际返回的符合条件的 EGLConfig 数

/*
EGLint attribList[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE, 5,
    EGL_GREEN_SIZE, 6,
    EGL_BLUE_SIZE, 5,
    EGL_DEPTH_SIZE, 1,
    EGL_NONE
};

const EGLint MaxConfigs = 10;
EGLConfig configs[MaxConfigs]; // We'll only accept 10 configs
EGLint numConfigs;
if (!eglChooseConfig(dpy, attribList, configs, MaxConfigs, &numConfigs)) {
    // Something didn't work … handle error situation
} else {
    // Everything's okay. Continue to create a rendering surface
}
*/

// eglCreatePbufferSurface()

EGLSurface eglCreateWindowSurface(EGLDisplay display, // 指定显示的连接
    EGLConfig config, // 符合条件的 EGLConfig
    EGLNativeWindowType window, // 指定原生窗口
    const EGLint *attribList); // 指定窗口属性列表，可为 NULL

/*
EGLint attribList[] = {
  EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
  EGL_NONE
);

// Android 中的输出窗口通常为 ANativeWindow 对象。可以通过 Surface 类创建
ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
EGLSurface eglSurface = eglCreateWindowSurface(display, config, window, attribList);

if (eglSurface == EGL_NO_SURFACE) {
    switch (eglGetError()) {
    case EGL_BAD_MATCH:
        // Check window and EGLConfig attributes to determine
        // compatibility, or verify that the EGLConfig
        // supports rendering to a window,
        break;
    case EGL_BAD_CONFIG:
        // Verify that provided EGLConfig is valid
        break;
    case EGL_BAD_NATIVE_WINDOW:
        // Verify that provided EGLNativeWindow is valid
        break;
    case EGL_BAD_ALLOC:
        // Not enough resources available. Handle and recover
        break;
    }
}
*/

EGLContext eglCreateContext(EGLDisplay display, // 指定显示的连接
    EGLConfig config, // 前面选好的 EGLConfig
    EGLContext shareContext, // 允许其它 EGLContext 共享数据，使用 EGL_NO_CONTEXT 表示不共享
    const EGLint* attribList); // 指定操作的属性列表，只能接受一个属性 EGL_CONTEXT_CLIENT_VERSION

/*
const ELGint attribList[] = {
    EGL_CONTEXT_CLIENT_VERSION, 3,
    EGL_NONE
};

EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, attribList);

if (context == EGL_NO_CONTEXT) {
    EGLError error = eglGetError();
    if (error == EGL_BAD_CONFIG) {
        // Handle error and recover
    }
}
*/

EGLBoolean eglMakeCurrent(EGLDisplay display, // 指定显示的连接
    EGLSurface draw, // EGL 绘图表面
    EGLSurface read, // EGL 读取表面
    EGLContext context); // 指定连接到该表面的上下文

/*
// 创建 GLES 环境
int BgRender::CreateGlesEnv()
{
    // EGL config attributes
    const EGLint confAttr[] =
    {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
            EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,//EGL_WINDOW_BIT EGL_PBUFFER_BIT we will create a pixelbuffer surface
            EGL_RED_SIZE,   8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE,  8,
            EGL_ALPHA_SIZE, 8,// if you need the alpha channel
            EGL_DEPTH_SIZE, 8,// if you need the depth buffer
            EGL_STENCIL_SIZE,8,
            EGL_NONE
    };
 
    // EGL context attributes
    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
 
    // surface attributes
    // the surface size is set to the input frame size
    const EGLint surfaceAttr[] = {
            EGL_WIDTH, 1,
            EGL_HEIGHT,1,
            EGL_NONE
    };
    EGLint eglMajVers, eglMinVers;
    EGLint numConfigs;
 
    int resultCode = 0;
    do
    {
        // 1. 获取 EGLDisplay 对象，建立与本地窗口系统的连接
        m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if(m_eglDisplay == EGL_NO_DISPLAY)
        {
            //Unable to open connection to local windowing system
            LOGCATE("BgRender::CreateGlesEnv Unable to open connection to local windowing system");
            resultCode = -1;
            break;
        }
 
        // 2. 初始化 EGL 方法
        if(!eglInitialize(m_eglDisplay, &eglMajVers, &eglMinVers))
        {
            // Unable to initialize EGL. Handle and recover
            LOGCATE("BgRender::CreateGlesEnv Unable to initialize EGL");
            resultCode = -1;
            break;
        }
 
        LOGCATE("BgRender::CreateGlesEnv EGL init with version %d.%d", eglMajVers, eglMinVers);
 
        // 3. 获取 EGLConfig 对象，确定渲染表面的配置信息
        if(!eglChooseConfig(m_eglDisplay, confAttr, &m_eglConf, 1, &numConfigs))
        {
            LOGCATE("BgRender::CreateGlesEnv some config is wrong");
            resultCode = -1;
            break;
        }
 
        // 4. 创建渲染表面 EGLSurface, 使用 eglCreatePbufferSurface 创建屏幕外渲染区域
        m_eglSurface = eglCreatePbufferSurface(m_eglDisplay, m_eglConf, surfaceAttr);
        if(m_eglSurface == EGL_NO_SURFACE)
        {
            switch(eglGetError())
            {
                case EGL_BAD_ALLOC:
                    // Not enough resources available. Handle and recover
                    LOGCATE("BgRender::CreateGlesEnv Not enough resources available");
                    break;
                case EGL_BAD_CONFIG:
                    // Verify that provided EGLConfig is valid
                    LOGCATE("BgRender::CreateGlesEnv provided EGLConfig is invalid");
                    break;
                case EGL_BAD_PARAMETER:
                    // Verify that the EGL_WIDTH and EGL_HEIGHT are
                    // non-negative values
                    LOGCATE("BgRender::CreateGlesEnv provided EGL_WIDTH and EGL_HEIGHT is invalid");
                    break;
                case EGL_BAD_MATCH:
                    // Check window and EGLConfig attributes to determine
                    // compatibility and pbuffer-texture parameters
                    LOGCATE("BgRender::CreateGlesEnv Check window and EGLConfig attributes");
                    break;
            }
        }
 
        // 5. 创建渲染上下文 EGLContext
        m_eglCtx = eglCreateContext(m_eglDisplay, m_eglConf, EGL_NO_CONTEXT, ctxAttr);
        if(m_eglCtx == EGL_NO_CONTEXT)
        {
            EGLint error = eglGetError();
            if(error == EGL_BAD_CONFIG)
            {
                // Handle error and recover
                LOGCATE("BgRender::CreateGlesEnv EGL_BAD_CONFIG");
                resultCode = -1;
                break;
            }
        }
 
        // 6. 绑定上下文
        if(!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglCtx))
        {
            LOGCATE("BgRender::CreateGlesEnv MakeCurrent failed");
            resultCode = -1;
            break;
        }
        LOGCATE("BgRender::CreateGlesEnv initialize success!");
    }
    while (false);
 
    if (resultCode != 0)
    {
        LOGCATE("BgRender::CreateGlesEnv fail");
    }
 
    return resultCode;
}
 
// 渲染
void BgRender::Draw()
{
    LOGCATE("BgRender::Draw");
    if (m_ProgramObj == GL_NONE) return;
    glViewport(0, 0, m_RenderImage.width, m_RenderImage.height);
 
    // Do FBO off screen rendering
    glUseProgram(m_ProgramObj);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
 
    glBindVertexArray(m_VaoIds[0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
    glUniform1i(m_SamplerLoc, 0);
 
    if (m_TexSizeLoc != GL_NONE) {
        GLfloat size[2];
        size[0] = m_RenderImage.width;
        size[1] = m_RenderImage.height;
        glUniform2fv(m_TexSizeLoc, 1, &size[0]);
    }
 
    // 7. 渲染
    GO_CHECK_GL_ERROR();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
    GO_CHECK_GL_ERROR();
    glBindVertexArray(GL_NONE);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);
 
    // 一旦解绑 FBO 后面就不能调用 readPixels
    // glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
 
}
 
// 释放 GLES 环境
void BgRender::DestroyGlesEnv()
{
    // 8. 释放 EGL 环境
    if (m_eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(m_eglDisplay, m_eglCtx);
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        eglReleaseThread();
        eglTerminate(m_eglDisplay);
    }
 
    m_eglDisplay = EGL_NO_DISPLAY;
    m_eglSurface = EGL_NO_SURFACE;
    m_eglCtx = EGL_NO_CONTEXT;
}
*/

/*
// 创建渲染对象
NativeBgRender mBgRender = new NativeBgRender();
// 初始化创建 GLES 环境
mBgRender.native_BgRenderInit();
// 加载图片数据到纹理
loadRGBAImage(R.drawable.java, mBgRender);
// 离屏渲染
mBgRender.native_BgRenderDraw();
// 从缓冲区读出渲染后的图像数据，加载到 ImageView
mImageView.setImageBitmap(createBitmapFromGLSurface(0, 0, 421, 586));
// 释放 GLES 环境
mBgRender.native_BgRenderUnInit();

private void loadRGBAImage(int resId, NativeBgRender render) {
    InputStream is = this.getResources().openRawResource(resId);
    Bitmap bitmap;
    try {
        bitmap = BitmapFactory.decodeStream(is);
        if (bitmap != null) {
            int bytes = bitmap.getByteCount();
            ByteBuffer buf = ByteBuffer.allocate(bytes);
            bitmap.copyPixelsToBuffer(buf);
            byte[] byteArray = buf.array();
            render.native_BgRenderSetImageData(byteArray, bitmap.getWidth(), bitmap.getHeight());
        }
    }
    finally
    {
        try
        {
            is.close();
        }
        catch(IOException e)
        {
            e.printStackTrace();
        }
    }
}
 
private Bitmap createBitmapFromGLSurface(int x, int y, int w, int h) {
    int bitmapBuffer[] = new int[w * h];
    int bitmapSource[] = new int[w * h];
    IntBuffer intBuffer = IntBuffer.wrap(bitmapBuffer);
    intBuffer.position(0);
    try {
        GLES20.glReadPixels(x, y, w, h, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE,
                intBuffer);
        int offset1, offset2;
        for (int i = 0; i < h; i++) {
            offset1 = i * w;
            offset2 = (h - i - 1) * w;
            for (int j = 0; j < w; j++) {
                int texturePixel = bitmapBuffer[offset1 + j];
                int blue = (texturePixel >> 16) & 0xff;
                int red = (texturePixel << 16) & 0x00ff0000;
                int pixel = (texturePixel & 0xff00ff00) | red | blue;
                bitmapSource[offset2 + j] = pixel;
            }
        }
    } catch (GLException e) {
        return null;
    }
    return Bitmap.createBitmap(bitmapSource, w, h, Bitmap.Config.ARGB_8888);
}
*/

// X Window <-> glx <-> OpenGL
// MS Windows <-> wgl <-> OpenGL
// macOS <-> agl <-> OpenGL
// Android(All Embedding System) <-> egl <-> OpenGL ES
// DRI (Direct Rendering Infrastructure)
// XGL <-> (Xglx | Xegl) <-> (glx | egl) <-> (xserver | ...)
// AIGLX (Accelerated Indirect GLX) xclient <-opengl-> xserver <-> mesa(OpenGL)
