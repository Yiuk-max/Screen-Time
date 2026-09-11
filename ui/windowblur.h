#ifndef WINDOWBLUR_H
#define WINDOWBLUR_H

class QWidget;

// 原生标题栏外观辅助：深色标题栏 + 圆角窗口。
//
// 「极夜极光」是接近实体的深色界面，不再使用 Acrylic / Mica 背景模糊，
// 这里只负责让系统标题栏与主题深浅一致。
namespace WindowBlur
{
void applyFrame(QWidget *window, bool dark);
}

#endif // WINDOWBLUR_H
