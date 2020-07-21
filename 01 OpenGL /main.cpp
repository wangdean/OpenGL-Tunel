#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLFrame.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

// 纹理标识符号
#define TEXTURE_BRICK   0 //墙面
#define TEXTURE_FLOOR   1 //地板
#define TEXTURE_CEILING 2 //纹理天花板
#define TEXTURE_COUNT   3 //纹理个数

GLuint  textures[TEXTURE_COUNT];//纹理标记数组
//文件tag名字数组
const char *szTextureFiles[TEXTURE_COUNT] = { "brick.tga", "floor.tga", "ceiling.tga" };

GLShaderManager        shaderManager;            //着色器管理器
GLMatrixStack          modelViewMatrix;          //模型视图矩阵
GLMatrixStack          projectionMatrix;         //投影矩阵
GLFrustum              viewFrustum;              //视景体
GLGeometryTransform    transformPipeline;        //几何变换管线

//4个批次容器类
GLBatch             floorBatch;//地面
GLBatch             ceilingBatch;//天花板
GLBatch             leftWallBatch;//左墙面
GLBatch             rightWallBatch;//右墙面

//深度初始值，-65。
GLfloat             viewZ = -65.0f;

void changeSize(int width,int height)
{
    if(height == 0)
        height = 1;
    
    //重新设置视口大小
    glViewport(0, 0, width, height);
    
    GLfloat factor = (GLfloat)width/(GLfloat)height;
    
    viewFrustum.SetPerspective(80.0f, factor, 1.0, 120.0);
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    transformPipeline.SetMatrixStacks(modelViewMatrix,projectionMatrix);
}

void SpecialKeys(int key,int x,int y)
{
    if(key == GLUT_KEY_UP)
    {
        viewZ += 0.5f;
    }
    
    if(key == GLUT_KEY_DOWN)
    {
        viewZ -= 0.5f;
    }
    
    //重新绘制
    glutPostRedisplay();
}

//这个函数用来初始化渲染环境，设置并初始化纹理对象
void setUpRC()
{
    //设置背景颜色为黑色
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //初始化着色器管理类
    shaderManager.InitializeStockShaders();
    
    GLbyte * pBytes;
    GLint iWidth,iHeight,iComponets;
    GLenum eFormet;
    GLint iLoop;
    
    //3.生成纹理标记,执行结束后 textures 会填充为纹理id
    /** 分配纹理对象 glGenTextures
     参数1：纹理对象的数量
     参数2：纹理对象标识数组
     */
    glGenTextures(TEXTURE_COUNT, textures);
    
    //循环设置纹理参数，因为有三个纹理，所以这里采用循环去设置
    for(iLoop = 0;iLoop < TEXTURE_COUNT;iLoop++)
    {
        //绑定纹理对象
        glBindTexture(GL_TEXTURE_2D, textures[iLoop]);
        //加载tga文件，加载完成后 iWidth,iHeight,iComponets 等参数都会被填充为本tga文件的属性
        pBytes = gltReadTGABits(szTextureFiles[iLoop], &iWidth, &iHeight, &iComponets, &eFormet);
        
        //设置纹理放大缩小时的过滤方式为邻近过滤
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //设置纹理在s轴的环绕方式为 GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        
        //设置纹理在T轴的环绕方式为 GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        /**载入纹理 glTexImage2D
        参数1：纹理维度，GL_TEXTURE_2D
        参数2：mip贴图层次
        参数3：纹理单元存储的颜色成分（从读取像素图中获得）
        参数4：加载纹理宽度
        参数5：加载纹理的高度
        参数6：加载纹理的深度
        参数7：像素数据的数据类型,GL_UNSIGNED_BYTE无符号整型
        参数8：指向纹理图像数据的指针
        */
        glTexImage2D(GL_TEXTURE_2D, 0, iComponets, iWidth, iHeight, 0, eFormet, GL_UNSIGNED_BYTE, pBytes);
        
        /**为纹理对象生成一组完整的mipmap glGenerateMipmap
             参数1：纹理维度，GL_TEXTURE_1D,GL_TEXTURE_2D,GL_TEXTURE_2D
         */
        glGenerateMipmap(GL_TEXTURE_2D);
        
        //释放原始纹理数据，不在需要纹理原始数据了
        free(pBytes);
    }
    
    //纹理当前的z坐标
    GLfloat z;
    
    /*
    GLTools库中的容器类，GBatch，
    void GLBatch::Begin(GLenum primitive,GLuint nVerts,GLuint nTextureUnits = 0);
    参数1：图元枚举值
    参数2：顶点数
    参数3：1组或者2组纹理坐标
    */
    floorBatch.Begin(GL_TRIANGLE_STRIP, 28,1);
    for(z = 60;z >= 0.0f;z -= 10.0f)
    {
        floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        floorBatch.Vertex3f(-10.0f, -10.0f, z);
        
        floorBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        floorBatch.Vertex3f(10.0f, -10.0f, z);
        
        floorBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        floorBatch.Vertex3f(-10.0f, -10.0f, z - 10.0f);
        
        floorBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        floorBatch.Vertex3f(10.0f, -10.0f, z - 10.0f);
    }
    floorBatch.End();
    
    ceilingBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f)
    {
        ceilingBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        ceilingBatch.Vertex3f(-10.0f, 10.0f, z - 10.0f);
        
        ceilingBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        ceilingBatch.Vertex3f(10.0f, 10.0f, z - 10.0f);
        
        ceilingBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        ceilingBatch.Vertex3f(-10.0f, 10.0f, z);
        
        ceilingBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        ceilingBatch.Vertex3f(10.0f, 10.0f, z);
    }
    ceilingBatch.End();
    
    leftWallBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f)
    {
        leftWallBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        leftWallBatch.Vertex3f(-10.0f, -10.0f, z);
        
        leftWallBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        leftWallBatch.Vertex3f(-10.0f, 10.0f, z);
        
        leftWallBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        leftWallBatch.Vertex3f(-10.0f, -10.0f, z - 10.0f);
        
        leftWallBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        leftWallBatch.Vertex3f(-10.0f, 10.0f, z - 10.0f);
    }
    leftWallBatch.End();
    
    rightWallBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f)
    {
        rightWallBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        rightWallBatch.Vertex3f(10.0f, -10.0f, z);
        
        rightWallBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        rightWallBatch.Vertex3f(10.0f, 10.0f, z);
        
        rightWallBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        rightWallBatch.Vertex3f(10.0f, -10.0f, z - 10.0f);
        
        rightWallBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        rightWallBatch.Vertex3f(10.0f, 10.0f, z - 10.0f);
    }
    rightWallBatch.End();
}

void shutDownRC()
{
    //删除纹理,本次绘制结束后，需要删除纹理，否则会影响其他绘制
    glDeleteTextures(TEXTURE_COUNT, textures);
}

void RenderScene()
{
    //清除颜色缓冲区
    glClear(GL_COLOR_BUFFER_BIT);
    
    //模型视图矩阵压栈
    modelViewMatrix.PushMatrix();
    //z轴平移viewZ距离
    modelViewMatrix.Translate(0.0f, 0.0f, viewZ);
    
    //纹理替换矩阵着色器
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_REPLACE,transformPipeline.GetModelViewProjectionMatrix(),0);
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_FLOOR]);
    floorBatch.Draw();
    
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_CEILING]);
    ceilingBatch.Draw();
    
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_BRICK]);
    leftWallBatch.Draw();
    rightWallBatch.Draw();
    
    modelViewMatrix.PopMatrix();
    glutSwapBuffers();
}

//菜单栏选择
void ProcessMenu(int value)
{
    GLint iLoop;
    
    for(iLoop = 0; iLoop < TEXTURE_COUNT; iLoop++)
    {
        /**绑定纹理 glBindTexture
         参数1：GL_TEXTURE_2D
         参数2：需要绑定的纹理对象
         */
        glBindTexture(GL_TEXTURE_2D, textures[iLoop]);
        
        /**配置纹理参数 glTexParameteri
         参数1：纹理模式
         参数2：纹理参数
         参数3：特定纹理参数
         
         */
        switch(value)
        {
            case 0:
                //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST（最邻近过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                break;
                
            case 1:
                //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_LINEAR（线性过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                break;
                
            case 2:
                //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST_MIPMAP_NEAREST（选择最邻近的Mip层，并执行最邻近过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
                break;
                
            case 3:
                //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST_MIPMAP_LINEAR（在Mip层之间执行线性插补，并执行最邻近过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                break;
                
            case 4:
                //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST_MIPMAP_LINEAR（选择最邻近Mip层，并执行线性过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                break;
                
            case 5:
                //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_LINEAR_MIPMAP_LINEAR（在Mip层之间执行线性插补，并执行线性过滤，又称为三线性过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                break;
                
            case 6:
            
                //设置各向异性过滤
                GLfloat fLargest;
                //获取各向异性过滤的最大数量
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
                //设置纹理参数(各向异性采样)
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
                break;
        
            case 7:
                //设置各向同性过滤，数量为1.0表示(各向同性采样)
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
                break;
                
        }
    }
    
    //触发重画
    glutPostRedisplay();
}

int main(int argc, char *argv[])
{
    gltSetWorkingDirectory(argv[0]);
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    //设置视口大小
    glutInitWindowSize(800, 600);
    //创建一个window
    glutCreateWindow("Tunel");
    //设置窗口大小改变的回调函数
    glutReshapeFunc(changeSize);
    //设置键盘输入的回调
    glutSpecialFunc(SpecialKeys);
    //设置绘制的回调函数
    glutDisplayFunc(RenderScene);
    
    // 添加菜单入口，改变过滤器
    glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("GL_NEAREST",0);
    glutAddMenuEntry("GL_LINEAR",1);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_NEAREST",2);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_LINEAR", 3);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_NEAREST", 4);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_LINEAR", 5);
    glutAddMenuEntry("Anisotropic Filter", 6);
    glutAddMenuEntry("Anisotropic Off", 7);
    //右键时显示菜单
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    GLenum err = glewInit();
    if(err != GLEW_OK)
    {
        fprintf(stderr, "GLEW Error: %s\n",glewGetErrorString(err));
        return 1;
    }
    
    setUpRC();
    glutMainLoop();
    shutDownRC();
    
    return 0;
}



