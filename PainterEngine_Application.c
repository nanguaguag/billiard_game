#include "PainterEngine_Application.h"
#include <math.h>

PX_Application App;

PX_FontModule fm;

PX_Object *root;

////////////////////// colors /////////////////////////

px_color orange, green, blue, yellow, white, black, grey, brown, shadowColor;

////////////////////// vector /////////////////////////
typedef px_point2D vector;

vector vectorAdd(vector a, vector b) {
    vector result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

vector vectorMinus(vector a, vector b) {
    vector result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

vector vectorMultiply(vector a, double n) {
    vector result;
    result.x = n * a.x;
    result.y = n * a.y;
    return result;
}

vector vectorDivide(vector a, double n) {
    vector result;
    result.x = a.x / n;
    result.y = a.y / n;
    return result;
}

vector vectorRotate90(vector a) {
    vector result;
    result.x = -a.y;
    result.y = a.x;
    return result;
}

double vectorDot(vector a, vector b) {
    return a.x * b.x + a.y * b.y;
}

double vectorAbs(vector a) {
    return sqrtf(a.x * a.x + a.y * a.y);
}

//void vectorPrint(vector a) {
//    printf("vector:(%f,", a.x);
//    printf("%f\n)", a.y);
//}

/////////////////////////Events/////////////////////////////

struct Mouse // 鼠标事件
{
    px_point2D pos;
} mouse;

px_bool pause = 0;
px_bool kickOffTime = 1;
px_bool onRuning = 0;
px_bool draging = 0;

////////////////////// shapes /////////////////////////
typedef struct {
    vector position;
    vector velocity;
    px_color color;
    px_int radius;
    px_double mass;
    px_bool inHole;
} physicsCircle;

// 16个球
physicsCircle circles[16];

////////////////////// other stuff ////////////////////

vector velocityReady;

// 明暗高亮圆圈正在变化changing=1
px_bool changing = 0;
// 明暗高亮圆圈的透明度（0 ~ 100 周期变化）
px_int colorAlpha = 0;
// 摩擦因数
double frictionConstant = 0.05;
// 蓝球进洞数
px_int blueBallsInHole = 0;
// 橙球进洞数
px_int orangeBallsInHole = 0;
// 6个洞的XY坐标
px_int holesPositionX[6] = {254, 890, 1526, 254, 890, 1526};
px_int holesPositionY[6] = {160, 160, 160, 796, 796, 796};
// 洞的半径
px_float holesRadius = 58;
// 轮到蓝方开球
px_bool blueTurn = 1;
// 获胜
px_bool blueWin = 0;
px_bool orangeWin = 0;
// 进球
px_bool cickedInBlue = 0;
px_bool cickedInOrange = 0;

/////////////////////// END ///////////////////////////

px_bool PX_ApplicationInitialize(PX_Application *pApp, px_int screen_width, px_int screen_height) {
    PX_ApplicationInitializeDefault(&pApp->runtime, screen_width, screen_height);
    if (!PX_FontModuleInitialize(&pApp->runtime.mp_resources, &fm)) return PX_FALSE;
    if (!PX_LoadFontModuleFromFile(&fm, "../assets/JetBrainsMono-Bold.pxf")) return PX_FALSE;
    root = PX_ObjectCreate(&pApp->runtime.mp_ui, PX_NULL, 0, 0, 0, 0, 0, 0);
    orange = PX_COLOR(255, 222, 143, 111);
    green = PX_COLOR(255, 72, 158, 104);
    blue = PX_COLOR(255, 62, 123, 171);
    yellow = PX_COLOR(255, 255, 255, 0);
    white = PX_COLOR(200, 255, 255, 255);
    black = PX_COLOR(255, 0, 0, 0);
    grey = PX_COLOR(200, 48, 48, 48);
    brown = PX_COLOR(255, 109, 70, 60);
    shadowColor = PX_COLOR(50, 48, 48, 48);
    // 统一属性
    px_bool isBlue[16] = {0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0};
    for (int i = 0; i < 16; ++i) {
        circles[i].mass = 1;
        circles[i].velocity.x = 0;
        circles[i].velocity.y = 0;
        circles[i].radius = 30;
        circles[i].color = isBlue[i] ? blue : orange;
        circles[i].inHole = 0;
    }
    // 前15个球的坐标和颜色（搞成三角形）
    for (int i = 0; i < 5; ++i) {
        circles[i].position.x = 610;
        circles[i].position.y = 359 + 60 * i;
    }
    for (int i = 1; i < 5; ++i) {
        int n = (11 * i - i * i) / 2;
        circles[n].position.x = circles[n - (6 - i)].position.x + circles[n].radius * 1.732051;
        circles[n].position.y = circles[n - (6 - i)].position.y + circles[n].radius;
        for (int j = 1; j < 5 - i; ++j) {
            circles[n + j].position.x = circles[n + j - 1].position.x;
            circles[n + j].position.y = circles[n + j - 1].position.y + 60;
        }
    }
    // 白球、黑球单独初始化
    circles[15].color = white;
    circles[15].position.x = 1117;
    circles[15].position.y = 477;
    circles[10].color = grey;
    return PX_TRUE;
}

px_void PX_ApplicationUpdate(PX_Application *pApp, px_dword elapsed) {
    // pause = 1即暂停
    if (pause) return;
    // 获取窗口长和宽
//    double windowX = pApp->runtime.surface_width;
//    double windowY = pApp->runtime.surface_height;

    // 复杂度为O(n^2)的碰撞检测
    for (int i = 0; i < 16; ++i) {
        if (circles[i].inHole) continue;
        for (int j = i + 1; j < 16; ++j) {
            if (circles[j].inHole) continue;
            if (pow(circles[j].position.x - circles[i].position.x, 2) +
                pow(circles[j].position.y - circles[i].position.y, 2) <
                pow(circles[i].radius + circles[j].radius, 2)) { // 如果发生碰撞
                vector M = {.x=circles[j].position.x - circles[i].position.x,
                        circles[j].position.y - circles[i].position.y};// M向量为两球连线方向的向量
                vector N = {.x=circles[i].position.x - circles[j].position.x,
                        circles[i].position.y - circles[j].position.y};
                // 分别计算两球速度在M向量上的投影向量
                vector c1m = vectorMultiply(M, vectorDot(circles[i].velocity, M) / vectorDot(M, M));
                vector c2m = vectorMultiply(N, vectorDot(circles[j].velocity, N) / vectorDot(N, N));
                // 俩球互相远离<-->不需要考虑碰撞
                if (vectorDot(M, c1m) <= 0 && vectorDot(N, c2m) <= 0)
                    continue;
                // 分别计算两球速度在垂直M向量上的投影向量（碰撞前后不发生改变）
                vector c1n = vectorMinus(circles[i].velocity, c1m);
                vector c2n = vectorMinus(circles[j].velocity, c2m);
                // 计算碰撞后小球在M向量上的投影向量
                M = vectorDivide(vectorAdd(vectorMultiply(c2m, 2 * circles[j].mass),
                                           vectorMultiply(c1m, circles[i].mass - circles[j].mass)),
                                 circles[j].mass + circles[i].mass);
                c2m = vectorDivide(vectorAdd(vectorMultiply(c1m, 2 * circles[i].mass),
                                             vectorMultiply(c2m, circles[j].mass - circles[i].mass)),
                                   circles[i].mass + circles[j].mass);
                c1m = M;
                // 与之前不变的法向量合成
                circles[i].velocity = vectorAdd(c1m, c1n);
                circles[j].velocity = vectorAdd(c2m, c2n);
                // pause = 1;
            }
        }
    }
    blueWin = 1;
    orangeWin = 1;
    kickOffTime = 1;
    for (int i = 0; i < 16; ++i) {
        if (!circles[i].inHole) {
            if (circles[i].color._argb.r == 222) orangeWin = 0;
            else if (circles[i].color._argb.r == 62) blueWin = 0;
        }
        // 判断是否有可能进洞（粗检测）
        // 254, 890, 1526, 254, 890, 1526
        // 160, 160, 160,  796, 796, 796
        if (!(circles[i].position.x > 254 + holesRadius && circles[i].position.x < 1526 - holesRadius &&
              circles[i].position.y > 160 + holesRadius && circles[i].position.y < 796 - holesRadius)) {
            // 出了矩形框，有可能进洞
            int j = (int) (circles[i].position.x - 254) / 509 + ((int) (circles[i].position.y - 160) / 368) * 3;
            // 0 <= j <= 5 为洞的编号
            if (((j == 1 || j == 4) &&
                 pow(holesPositionX[j] - circles[i].position.x, 2) + pow(holesPositionY[j] - circles[i].position.y, 2)
                 <= (holesRadius * holesRadius)) || ((j == 0 || j == 2 || j == 3 || j == 5) &&
                                                     pow(holesPositionX[j] - circles[i].position.x, 2) +
                                                     pow(holesPositionY[j] - circles[i].position.y, 2)
                                                     <= ((holesRadius + circles[i].radius) *
                                                         (holesRadius + circles[i].radius)))) {// 精检测
                vector tmp = {.x=holesPositionX[j] - circles[i].position.x,
                        .y=holesPositionY[j] - circles[i].position.y};
                circles[i].velocity = vectorDivide(vectorAdd(tmp, circles[i].velocity), 50);
                circles[i].radius--;
                if (!circles[i].inHole) {
                    if (circles[i].color._argb.r == 222) {
                        orangeBallsInHole++;
                        cickedInOrange = 1;
                    }
                    else if (circles[i].color._argb.r == 62) {
                        blueBallsInHole++;
                        cickedInBlue = 1;
                    }
                }
                circles[i].inHole = 1;
//                circles[i].velocity.x = 0;
//                circles[i].velocity.y = 0;
            }
        }
        // 通过速度更新位置
        circles[i].position = vectorAdd(circles[i].position, circles[i].velocity);
        if ((circles[i].velocity.x == 0 && circles[i].velocity.y == 0) || circles[i].inHole)
            continue; // 速度为0或者已经进洞了就直接跳过，以免出现1/0的情况

        // 判断球是否越过边框
        px_bool isCollide = 0;
        if (circles[i].position.x + circles[i].radius >= 1525) {
            circles[i].velocity.x = -circles[i].velocity.x;// 如果超过左右界，速度X取反
            isCollide = 1;
            circles[i].position.x = 1525 - circles[i].radius;
        } else if (circles[i].position.x - circles[i].radius <= 256) {
            circles[i].velocity.x = -circles[i].velocity.x;// 如果超过左右界，速度X取反
            isCollide = 1;
            circles[i].position.x = 256 + circles[i].radius;
        }
        if (circles[i].position.y - circles[i].radius <= 172) {
            circles[i].velocity.y = -circles[i].velocity.y;// 如果超过上下界，速度Y取反
            isCollide = 1;
            circles[i].position.y = 172 + circles[i].radius;
        } else if (circles[i].position.y + circles[i].radius >= 782) {
            circles[i].velocity.y = -circles[i].velocity.y;// 如果超过上下界，速度Y取反
            isCollide = 1;
            circles[i].position.y = 782 - circles[i].radius;
        }

        // 摩擦力算法实现
        kickOffTime = 0;// velocity全为0就是开球时刻(kickOffTime=1)
        vector tmp = circles[i].velocity;
        if (isCollide) tmp = vectorMultiply(tmp, (1 - 2 / vectorAbs(tmp)));
        tmp = vectorMultiply(tmp, (1 - frictionConstant / vectorAbs(tmp)));
        if (vectorDot(tmp, circles[i].velocity) > 0) {
            // 防止出现摩擦力太大使小球反向运动
            circles[i].velocity = tmp;
        } else {
            circles[i].velocity.x = 0;
            circles[i].velocity.y = 0;
        }
    }
    if (onRuning == kickOffTime) // 当且仅当所有球(速度为0或进洞)时
    {
        // 换方发球
        if (blueTurn) {
            if (!cickedInBlue)
                blueTurn = !blueTurn;
        } else {
            if (!cickedInOrange)
                blueTurn = !blueTurn;
        }
        onRuning = 0;
        cickedInOrange = 0;
        cickedInBlue = 0;
    }
    if (circles[15].radius <= 10) {
        // 白球进洞 重新上台
        circles[15].position.x = 1117;
        circles[15].position.y = 477;
        circles[15].radius = 30;
        circles[15].velocity.x = 0;
        circles[15].velocity.y = 0;
        circles[15].inHole = 0;
    }
//    pause = 1;// 暂停
}

px_void PX_ApplicationRender(PX_Application *pApp, px_dword elapsed) {
    px_surface *pRenderSurface = &pApp->runtime.RenderSurface;
    PX_RuntimeRenderClear(&pApp->runtime, white);
    // 背景：桌球台
    PX_GeoDrawRect(pRenderSurface, 173, 84, 1607, 868, brown);
    PX_GeoDrawRect(pRenderSurface, 256, 172, 1525, 782, green);
    PX_GeoDrawBorder(pRenderSurface, 231, 147, 1550, 807, 25, black);
    PX_GeoDrawRect(pRenderSurface, 256, 730, 1527, 782, shadowColor);
    // 6个洞，坐标分别为
    // (x:254,   y:160) (x:890,   y:162) (x:1527,  y:159) (x:254,   y:794) (x:890,   y:797) (x:1525,  y:795)
    // for (int i = 0; i < 2; ++i) {
    //     for (int j = 0; j < 3; ++j) {
    //         PX_GeoDrawSolidCircle(pRenderSurface, 254 + 636 * j, 160 + 636 * i, holesRadius, black);
    //     }
    // }
    for (int i = 0; i < 6; ++i) {
        PX_GeoDrawSolidCircle(pRenderSurface, holesPositionX[i], holesPositionY[i], holesRadius, black);
    }
    // 画出每一个小球+其阴影
    for (int i = 0; i < 16; ++i) {
        if (circles[i].radius <= 10) continue;// 小于10说明已经进洞了
        // 阴影
        PX_GeoDrawSolidCircle(pRenderSurface, circles[i].position.x,
                              circles[i].position.y - (float) circles[i].radius / 2,
                              circles[i].radius, shadowColor);
        // 内部彩色
        PX_GeoDrawSolidCircle(pRenderSurface, circles[i].position.x,
                              circles[i].position.y, circles[i].radius, circles[i].color);
        // 外壳黑色
        PX_GeoDrawCircle(pRenderSurface, circles[i].position.x, circles[i].position.y,
                         circles[i].radius - 6, 12, black);
        // 表示速度的箭头
        PX_GeoDrawArrow(pRenderSurface, circles[i].position, vectorAdd(circles[i].position, vectorMultiply(
                circles[i].velocity, 5)), 1, yellow);
    }
    // 谁开球
    char content1[64], content2[64];
    if (blueTurn) {
        PX_GeoDrawSolidCircle(pRenderSurface, 0, 450, 150, blue);
        PX_sprintf0(content1, sizeof(content1), "Blue\nTurn");
        PX_FontModuleDrawText(pRenderSurface, &fm, 0, 450, PX_ALIGN_LEFTMID, content1, white);
    } else {
        PX_GeoDrawSolidCircle(pRenderSurface, 0, 450, 150, orange);
        PX_sprintf0(content2, sizeof(content2), "Orange\nTurn");
        PX_FontModuleDrawText(pRenderSurface, &fm, 0, 450, PX_ALIGN_LEFTMID, content2, white);
    }
    // 拖拽时的白色箭头和高亮圆圈
    if (kickOffTime && ((pow(mouse.pos.x - circles[15].position.x, 2) +
                         pow(mouse.pos.y - circles[15].position.y, 2)) <=
                        pow(circles[15].radius, 2) || draging)) {// 如果在发球时刻
        // 高亮圆圈的绘制
        PX_GeoDrawCircle(pRenderSurface, circles[15].position.x, circles[15].position.y,
                         circles[15].radius + 10, 20, PX_COLOR(colorAlpha, 255, 255, 0));
        velocityReady = vectorDivide(vectorMinus(mouse.pos, circles[15].position), 50);
        // 形成明暗相间的高亮圆圈
        if (colorAlpha == 100) changing = 0;
        else if (colorAlpha == 0) changing = 1;
        colorAlpha += changing * 8 - 4;// 透明度不断改变，改变速率为 4
        // 箭头的绘制
        if (draging) {// 如果在拖球
            // 箭头柄
            vector tmp = vectorRotate90(velocityReady);
            PX_GeoDrawTriangle(pRenderSurface, circles[15].position,
                               vectorAdd(mouse.pos, tmp), vectorMinus(mouse.pos, tmp), white);
            // 箭头尖端三角形
            // 满足黄金分割比，看上去应该舒服些 >>> 6.472136=2*(sqrt(5)+1)
            PX_GeoDrawTriangle(pRenderSurface, vectorAdd(mouse.pos, vectorMultiply(velocityReady, 6.472136)),
                               vectorAdd(mouse.pos, vectorMultiply(tmp, 2)),
                               vectorMinus(mouse.pos, vectorMultiply(tmp, 2)), white);
        }
    }

    PX_sprintf1(content1, sizeof(content1), "Blue In Hole: < %1 >", PX_STRINGFORMAT_INT(blueBallsInHole));
    PX_sprintf1(content2, sizeof(content2), "Orange In Hole: < %1 >", PX_STRINGFORMAT_INT(orangeBallsInHole));
    PX_FontModuleDrawText(pRenderSurface, &fm, pApp->runtime.surface_width, 5, PX_ALIGN_RIGHTTOP, content1, blue);
    PX_FontModuleDrawText(pRenderSurface, &fm, 0, 5, PX_ALIGN_LEFTTOP, content2, orange);

    if (circles[10].inHole) {
        if (blueTurn && blueWin) {
            PX_GeoDrawRect(pRenderSurface, 0, 0, pApp->runtime.surface_width, pApp->runtime.surface_height,
                           PX_COLOR(50, 255, 255, 255));
            PX_sprintf0(content1, sizeof(content1), "Blue Win!!");
            PX_FontModuleDrawText(pRenderSurface, &fm, pApp->runtime.surface_width / 2,
                                  pApp->runtime.surface_height / 2, PX_ALIGN_CENTER, content1, blue);
        } else if (!blueTurn && orangeWin) {
            PX_GeoDrawRect(pRenderSurface, 0, 0, pApp->runtime.surface_width, pApp->runtime.surface_height,
                           PX_COLOR(50, 255, 255, 255));
            PX_sprintf0(content2, sizeof(content2), "Orange Win!!");
            PX_FontModuleDrawText(pRenderSurface, &fm, pApp->runtime.surface_width / 2,
                                  pApp->runtime.surface_height / 2, PX_ALIGN_CENTER, content2, orange);
        } else if (blueTurn && !blueWin) {
            PX_GeoDrawRect(pRenderSurface, 0, 0, pApp->runtime.surface_width, pApp->runtime.surface_height,
                           PX_COLOR(50, 255, 0, 0));
            PX_sprintf0(content1, sizeof(content1), "Blue Lose!!");
            PX_FontModuleDrawText(pRenderSurface, &fm, pApp->runtime.surface_width / 2,
                                  pApp->runtime.surface_height / 2, PX_ALIGN_CENTER, content1, blue);
        } else if (!blueTurn && !orangeWin) {
            PX_GeoDrawRect(pRenderSurface, 0, 0, pApp->runtime.surface_width, pApp->runtime.surface_height,
                           PX_COLOR(50, 255, 0, 0));
            PX_sprintf0(content2, sizeof(content2), "Orange Lose!!");
            PX_FontModuleDrawText(pRenderSurface, &fm, pApp->runtime.surface_width / 2,
                                  pApp->runtime.surface_height / 2, PX_ALIGN_CENTER, content2, orange);
        }
        pause = 1;
    }
    PX_ObjectRender(pRenderSurface, root, elapsed);
}

px_void PX_ApplicationPostEvent(PX_Application *pApp, PX_Object_Event e) {
//    PX_ApplicationEventDefault(&pApp->runtime, e);
    // 更新鼠标坐标
    mouse.pos.x = PX_Object_Event_GetCursorX(e);
    mouse.pos.y = PX_Object_Event_GetCursorY(e);
    if (e.Event == PX_OBJECT_EVENT_CURSORDOWN)
        draging = 1;
    if (e.Event == PX_OBJECT_EVENT_CURSORUP) {
        draging = 0;
        if (kickOffTime) {
            circles[15].velocity = velocityReady;
            colorAlpha = 0;
            kickOffTime = 0;
            onRuning = 1;
        }
    }
}

