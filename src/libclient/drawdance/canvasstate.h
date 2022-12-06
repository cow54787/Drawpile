#ifndef DRAWDANCE_CANVASSTATE_H
#define DRAWDANCE_CANVASSTATE_H

extern "C" {
#include <dpengine/load.h>
}

#include "annotationlist.h"
#include "documentmetadata.h"
#include "layercontent.h"
#include "layerlist.h"
#include "message.h"
#include "tile.h"
#include "timeline.h"
#include <QImage>
#include <QSize>

struct DP_CanvasState;

namespace drawdance {

class CanvasState final {
public:
    static CanvasState null();
    static CanvasState inc(DP_CanvasState *cs);
    static CanvasState noinc(DP_CanvasState *cs);

    static CanvasState load(const QString &path, DP_LoadResult *outResult = nullptr);

    CanvasState();
    CanvasState(const CanvasState &other);
    CanvasState(CanvasState &&other);

    CanvasState &operator=(const CanvasState &other);
    CanvasState &operator=(CanvasState &&other);

    ~CanvasState();

    DP_CanvasState *get() const;

    bool isNull() const;

    int width() const;
    int height() const;
    QSize size() const;

    Tile backgroundTile() const;
    DocumentMetadata documentMetadata() const;
    LayerList layers() const;
    AnnotationList annotations() const;
    Timeline timeline() const;

    int frameCount() const;

    QImage toFlatImage(
        bool includeBackground = true, bool includeSublayers = true) const;

    QImage toFlatImageArea(
        const QRect &rect, bool includeBackground = true,
        bool includeSublayers = true) const;

    QImage layerToFlatImage(int layerId, const QRect &rect) const;

    void toResetImage(MessageList &msgs, uint8_t contextId) const;

    drawdance::Message makeLayerOrder(
        uint8_t contextId, int sourceId, int targetId, bool intoGroup, bool below) const;

    LayerContent searchLayerContent(int layerId) const;

    int pickLayer(int x, int y) const;

    unsigned int pickContextId(int x, int y) const;

    QImage floodFill(
        int x, int y, const QColor &fillColor, double tolerance, int layerId,
        bool sampleMerged, int sizeLimit, int expand, int &outX, int &outY) const;

private:
    explicit CanvasState(DP_CanvasState *cs);

    static void pushMessage(void *user, DP_Message *msg);

    DP_CanvasState *m_data;
};

}

#endif
