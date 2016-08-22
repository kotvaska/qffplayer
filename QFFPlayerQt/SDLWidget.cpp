#include <iostream>
#include <QtWidgets/QApplication>
#include <QDesktopWidget>

#include "SDLWidget.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

void QSDLScreenWidget::mouseReleaseEvent(QMouseEvent *event) {
	isStop = true;
	changeCamera();
}

void QSDLScreenWidget::changeCamera(int count) {
	if (count > COUNT_CAM) count = 0;
	camera = count;
	if (!timer.isActive()) {
		timer.start(500, this);
	}
	changeFlags();
} 

int QSDLScreenWidget::changeCamera() {
	if (++camera > COUNT_CAM) camera = 0;
	closeFfmpeg();
	changeFlags();
	return camera;
} 

void QSDLScreenWidget::changeFlags() {
	switch (camera) {
	case 0:
		strcpy(link, "rtsp://192.168.1.96:554/profile2");
		isStop = false;
		break;
	case 1:
		strcpy(link, "rtsp://192.168.1.97:554/profile2");
		isStop = false;
		break;
	case 2:
		strcpy(link, "rtsp://192.168.1.98:554/profile2");
		isStop = false;
		break;
	case COUNT_CAM:
		strcpy(link, "http://griev.ru:8585/");
		emit changeCam();
		return;
	default:
		break;
	}

	if (!isStop && openCamera()) {
		emit changeCam();
	}
}

char* QSDLScreenWidget::getLink() {
	return link;
}

int QSDLScreenWidget::getCamCount() {
	return camera;
}

void QSDLScreenWidget::stopPlaying() {
	isStop = true;
	if (timer.isActive()) {
		timer.stop();
	}
}

QSDLScreenWidget::QSDLScreenWidget(QWidget *parent) :
	QWidget(parent),
	screen(0) {
		// Turn off double buffering for this widget. Double buffering
		// interferes with the ability for SDL to be properly displayed
		// on the QWidget.
		setAttribute(Qt::WA_PaintOnScreen);
		setAttribute(Qt::WA_NativeWindow, true);
		setAttribute(Qt::WA_OpaquePaintEvent);
		setMouseTracking(true);

		link = new char[100];
		isStop = true;

		camera = 0;

		pFormatCtx = NULL;
		pCodecCtxOrig = NULL;
		pCodecCtx = NULL;
		pCodec = NULL;
		pFrame = NULL;
		bmp = NULL;
		sws_ctx = NULL;

		av_register_all();
		avcodec_register_all();
		avformat_network_init();
		changeCamera(camera);

		timer.start(100, this);
}

QSDLScreenWidget::~QSDLScreenWidget() {
	link = NULL;

	pFormatCtx = NULL;
	pCodecCtxOrig = NULL;
	pCodecCtx = NULL;
	pCodec = NULL;
	pFrame = NULL;
	bmp = NULL;
	sws_ctx = NULL;

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
	closeFfmpeg();
	timer.stop();
}

void QSDLScreenWidget::resizeEvent(QResizeEvent *) {
	// We could get a resize event at any time, so clean previous mode.
	// You do this because if you don't you wind up with two windows
	// on the desktop: the Qt application and the SDL window. This keeps
	// the SDL region synchronized inside the Qt widget and the subsequent
	// application.
	//
	screen = 0;
	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	// Set the new video mode with the new window size
	//
	char variable[64];
	sprintf(variable, "SDL_WINDOWID=0x%lx", winId());
	putenv(variable);

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0 || SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "Unable to init SDL: " << SDL_GetError() << std::endl;
		return;
	}

#ifndef __DARWIN__
	screen = SDL_SetVideoMode(QApplication::desktop()->width(), QApplication::desktop()->height(), 0, /*SDL_FULLSCREEN | */SDL_SWSURFACE);
#else
	screen = SDL_SetVideoMode(QApplication::desktop()->width(), QApplication::desktop()->height(), 24, /*SDL_FULLSCREEN |*/ SDL_SWSURFACE);
#endif
	if(!screen) {
		std::cerr << "Unable to set video mode: " << SDL_GetError() <<
			std::endl;
		return;
	}

	SDL_ShowCursor(SDL_DISABLE);
}

void QSDLScreenWidget::paintEvent(QPaintEvent *) {
#ifdef Q_WS_X11
	// Make sure we're not conflicting with drawing from the Qt library
	//
	XSync(QX11Info::display(), FALSE);
#endif

	if (screen) {

		if (bmp == NULL) {
			bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
				pCodecCtx->height,
				SDL_YV12_OVERLAY,
				screen);
		}

		packetCount = 0;
		isEnd = 0;
		// Read frames 
		while(!isStop && isEnd >= 0 && ++packetCount < 10) {
			isEnd = av_read_frame(pFormatCtx, &packet);

			if (isEnd < 0) {
				av_free_packet(&packet);
				closeFfmpeg();
				isStop = true;
				emit errorCam();
				return;
			}

			// Is this a packet from the video stream?
			if(packet.stream_index == videoStream) {
				// Decode video frame
				if (avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet) <= 0) {
					av_free_packet(&packet);
					continue;
				}
				// Did we get a video frame?
				if(frameFinished) {
					SDL_LockYUVOverlay(bmp);

					pict.data[0] = bmp->pixels[0];
					pict.data[1] = bmp->pixels[2];
					pict.data[2] = bmp->pixels[1];

					pict.linesize[0] = bmp->pitches[0];
					pict.linesize[1] = bmp->pitches[2];
					pict.linesize[2] = bmp->pitches[1];

					// Convert the image into YUV format that SDL uses
					sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
						pFrame->linesize, 0, pCodecCtx->height,
						pict.data, pict.linesize);

					SDL_UnlockYUVOverlay(bmp);

					rect.x = 0;
					rect.y = 0;
					rect.w = QApplication::desktop()->width();
					rect.h = QApplication::desktop()->height();
					SDL_DisplayYUVOverlay(bmp, &rect);

				}

			}

			// Free the packet that was allocated by av_read_frame
			av_free_packet(&packet);
		}

	}

}

void QSDLScreenWidget::timerEvent(QTimerEvent *ev) {
	if (ev->timerId() != timer.timerId()) {
		QWidget::timerEvent(ev);
		return;
	}

	if (isStop) {
		isStop = !openFfmpeg(link);
		if (isStop) {
			return;
		}
		closeFfmpeg();
		changeCamera(camera);
	}

	update();
}

bool QSDLScreenWidget::openCamera() {
	isStop = !openFfmpeg(link);

	if (isStop) {
		emit errorCam();
		return !isStop;
	}

	// Allocate a place to put our YUV image on that screen
	if (screen) {
		bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
			pCodecCtx->height,
			SDL_YV12_OVERLAY,
			screen);
	}

	// initialize SWS context for software scaling
	if (sws_ctx == NULL) {
		sws_ctx = sws_getContext(pCodecCtx->width,
			pCodecCtx->height,
			pCodecCtx->pix_fmt,
			pCodecCtx->width,
			pCodecCtx->height,
			AV_PIX_FMT_YUV420P,
			SWS_BILINEAR,
			NULL,
			NULL,
			NULL
			);
	}

	return !isStop;
}

QPaintEngine* QSDLScreenWidget::paintEngine() const {
	return NULL;
}

bool QSDLScreenWidget::openFfmpeg(const char *link) {
	AVDictionary *opts = NULL;
	if (av_dict_set(&opts, "rtsp_transport", "udp", 0) < 0) {
		return false;
	}

	// Open video file
	if(avformat_open_input(&pFormatCtx, link, NULL, NULL) != 0) {
		return false; // Couldn't open file
	}

	// Retrieve stream information
	if(avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		return false; // Couldn't find stream information
	}

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, link, 0);

	videoStream =- 1;
	// Find the first video stream
	if ((videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &pCodec, 0)) < 0) {
		return false;
	}

	// Find the first video stream
	videoStream =- 1;
	for(i = 0; i < pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
		if(videoStream == -1) {
			return false; // Didn't find a video stream
		}

		// Get a pointer to the codec context for the video stream
		pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
		// Find the decoder for the video stream
		pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
		if(pCodec == NULL) {
			fprintf(stderr, "Unsupported codec!\n");
			return false; // Codec not found
		}

		if(pCodec->capabilities & CODEC_CAP_TRUNCATED) {
			pCodecCtx->flags |= CODEC_FLAG_TRUNCATED;
		}

		// Copy context
		pCodecCtx = avcodec_alloc_context3(pCodec);
		if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
			fprintf(stderr, "Couldn't copy codec context");
			return false; // Error copying codec context
		}

		// Open codec
		if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
			return false; // Could not open codec
		}

		// Allocate video frame
		pFrame = av_frame_alloc();

		return true;
}

void QSDLScreenWidget::closeFfmpeg() {
	atexit(SDL_Quit);
	// Free the YUV frame
	av_frame_free(&pFrame);

	// Close the codec
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);

	// Close the video file
	avformat_close_input(&pFormatCtx);
	pFormatCtx = NULL;
	pFrame = NULL;
	bmp = NULL;
	sws_ctx = NULL;
}
