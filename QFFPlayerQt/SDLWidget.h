#ifndef SDLWIDGET_H
#define SDLWIDGET_H

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <Qt>
#include <QWidget>
#include <QTimer>

#ifdef __cplusplus
extern "C"{
#endif 

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#include <SDL.h>
#include <SDL_thread.h>

#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() */
#endif

#define COUNT_CAM 3

class QSDLScreenWidget : public QWidget
{
	Q_OBJECT

public:
	void changeCamera(int count);
	int changeCamera();
	bool openCamera();
	void closeFfmpeg();
	void stopPlaying();
	char* getLink();
	int getCamCount();

public:
	QSDLScreenWidget(QWidget *parent = 0);
	~QSDLScreenWidget();

protected:
	void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
	void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	QPaintEngine* paintEngine() const Q_DECL_OVERRIDE;
	void timerEvent(QTimerEvent *ev);

private:
	void changeFlags();
	bool openFfmpeg(const char *link);

signals:
	void changeCam();
	void errorCam();

private:
	char *link;
	bool isStop;
	int camera, packetCount, isEnd;
	QBasicTimer timer;

	AVFormatContext *pFormatCtx;
	int             i, videoStream;
	AVCodecContext  *pCodecCtxOrig;
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	AVFrame         *pFrame;
	AVPacket        packet;
	AVPicture		pict;
	int             frameFinished;
	float           aspect_ratio;
	struct SwsContext *sws_ctx;

	SDL_Overlay     *bmp;
	SDL_Surface     *screen;
	SDL_Rect        rect;
	SDL_Event       event;

};

#endif // SDLWIDGET_H
