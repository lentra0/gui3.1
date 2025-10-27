#include <iostream>
#include <cstring>

#include "window.h"
#include "image.h"
#include "GUI.h"
#include "myprog.h"

#define WIN_WIDTH   300
#define WIN_HEIGHT  200

class MainWindow : public Window
{
public:
    MainWindow();
    ~MainWindow();

    void OnCreate();
    bool OnKeyPress(uint64_t value);
    void OnSizeChanged();

private:
    Image *m_pImage;
};

MainWindow::MainWindow()
{
    m_ClassName = __FUNCTION__;
}

MainWindow::~MainWindow()
{
    // Внимание!
    // деструктор не является подходящим местом для удаления дочерних окон:
    // деструктор окна вызывается после удаления всех его дочерних окон
}

void MainWindow::OnCreate()
{
	Rect mysize = GetInteriorSize();

    IMAGEINFO ii = theGUI->LoadPNG("icons48/face-cool.png");
	m_pImage = new Image(ii);
    m_pImage->SetStyle(IMAGE_SCALE_FIT | IMAGE_ALIGNH_CENTER | IMAGE_ALIGNV_CENTER);
	AddChild(m_pImage, Point(0,0), mysize);

    CaptureKeyboard(this);
}

void MainWindow::OnSizeChanged()
{
	Rect mysize = GetInteriorSize();

	m_pImage->SetSize(mysize);
}

bool MainWindow::OnKeyPress(uint64_t keyval)
{
    switch(keyval)
    {
    case 'q':
        DeleteMe();
        return true;
    default:
        ;
    }
    return true;
}

int main(int argc, char **argv)
{
    MainWindow *pWindow = new MainWindow;

    int res = Run(argc, argv, pWindow, WIN_WIDTH, WIN_HEIGHT);

    delete pWindow;

    return res;
}

