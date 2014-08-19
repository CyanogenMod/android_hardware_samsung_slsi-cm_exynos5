#ifndef SC_LOGE
#error "This file must be included by libscaler.cpp"
#endif

#include <cstring>
#include <cstdlib>
#include "ExynosMutex.h"

#define SC_SRC_BUFTYPE V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
#define SC_DST_BUFTYPE V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE

class CScaler {
public:
    enum SC_EDGE { SC_SRC = 0, SC_DST = 1, SC_NUM_EDGES};
    enum { SC_MAX_PLANES = SC_NUM_OF_PLANES };
    enum { SC_MAX_NODENAME = 14 };

private:
    enum SC_FLAG {
        SCF_BUF_FRESH = 0,
        SCF_STREAMING,
        SCF_REQBUFS,
        SCF_QBUF,
        SCF_CACHEABLE,
        SCF_DRM,
        SCF_PREMULTIPLIED,
        SCF_RESERVED,
        SCF_BUFFER_FLAG_NUMBER = 8,

        SCF_CONTEXT_FLAG_BASE = SCF_BUFFER_FLAG_NUMBER * 2,
        SCF_ROTATION_FRESH = SCF_CONTEXT_FLAG_BASE,
        SCF_HFLIP,
        SCF_VFLIP,
        SCF_ALLOW_DRM,
        SCF_ROTATE_SHIFT,
        SCF_ROTATE_90 = SCF_ROTATE_SHIFT,
        SCF_ROTATE_180,
        SCF_NONBLOCKING,
        // rotation by 270 is combination of SCF_ROTATE_90 SCF_ROTATE_180
    };

#define SCF_ROTATE_BITS 3
#define SCF_ROTATE_MASK (SCF_ROTATE_BITS << SCF_ROTATE_SHIFT)

    struct FrameInfo {
        struct {
            v4l2_buf_type type;
            unsigned int width, height;
            unsigned int crop_left, crop_top, crop_width, crop_height;
            unsigned int color_format;
            void *addr[SC_MAX_PLANES];
            int fdAcquireFence;
            enum v4l2_memory memory;
            int out_num_planes;
            unsigned long out_plane_size[SC_MAX_PLANES];
        } edge[SC_NUM_EDGES];
        unsigned long flags;
    } m_Frame;

    int m_fdScaler;
    char m_cszNode[SC_MAX_NODENAME]; // /dev/videoXX
    int m_iInstance;

    int m_fdValidate;

    static const char *m_cszEdgeName[CScaler::SC_NUM_EDGES];

    void Initialize(int instance, bool nonblock);
    int ResetDevice(SC_EDGE edge);// streamoff, reqbufs(0)

    void SetFlag(SC_FLAG flag, int edge = SC_NUM_EDGES) {
        if (flag >= SCF_CONTEXT_FLAG_BASE) {
            m_Frame.flags |= 1 << flag;
        } else if (edge < SC_NUM_EDGES) {
            m_Frame.flags |= 1 << (flag + (edge * SCF_BUFFER_FLAG_NUMBER));
        }
    }

    void ClearFlag(SC_FLAG flag, int edge = SC_NUM_EDGES) {
        if (flag >= SCF_CONTEXT_FLAG_BASE) {
            m_Frame.flags &= ~(1 << flag);
        } else if (edge < SC_NUM_EDGES) {
            m_Frame.flags &= ~(1 << (flag + (edge * SCF_BUFFER_FLAG_NUMBER)));
        }
    }

    bool IsSet(SC_FLAG flag, int edge = SC_NUM_EDGES) {
        if (flag >= SCF_CONTEXT_FLAG_BASE) {
            return m_Frame.flags & (1 << flag);
        } else if (edge < SC_NUM_EDGES) {
            return m_Frame.flags & (1 << (flag + (edge * SCF_BUFFER_FLAG_NUMBER)));
        }
        return false;
    }

    int GetRotDegree() {

        return ((m_Frame.flags >> SCF_ROTATE_SHIFT) & SCF_ROTATE_BITS) * 90;
    }

    void SetRotDegree(int rot) {
        rot = rot % 360;
        if (rot < 0)
            rot = 360 + rot;

        rot /= 90;

        m_Frame.flags &= ~SCF_ROTATE_MASK;
        m_Frame.flags |= rot << SCF_ROTATE_SHIFT;
    }

public:
    bool Valid() { return (m_fdScaler >= 0) && (m_fdScaler == -m_fdValidate); }

    CScaler(int instance, int allow_drm = 0, bool nonblock = false);
    ~CScaler();

    bool IsDRMAllowed() { return IsSet(SCF_ALLOW_DRM); }
    int GetScalerID() { return m_iInstance; }

    int Stop();
    int Start(); // Blocking mode

    // H/W Control
    int SetCtrl();
    int SetFormat();
    int ReqBufs();
    int QBuf(); // no Android Sync support
    int QBuf(int pfdReleaseFence[SC_NUM_EDGES]); // Android Sync support
    int StreamOn();
    int DQBuf();
    int DQBuf(SC_EDGE edge);

    // Parameter Extraction
    void SetImgFormat(
        SC_EDGE edge,
        unsigned int width,
        unsigned int height,
        unsigned int crop_left,
        unsigned int crop_top,
        unsigned int crop_width,
        unsigned int crop_height,
        unsigned int v4l2_colorformat,
        unsigned int cacheable,
        unsigned int mode_drm,
        unsigned int premultiplied = 0);

    bool SetRotate(int rot, int flip_h, int flip_v);

    inline void SetAddr(SC_EDGE edge, void *addr[SC_NUM_OF_PLANES], int mem_type, int fence = -1)
    {
        for (int i = 0; i < SC_MAX_PLANES; i++)
            m_Frame.edge[edge].addr[i] = addr[i];

        m_Frame.edge[edge].memory = static_cast<v4l2_memory>(mem_type);
        m_Frame.edge[edge].fdAcquireFence = fence;
    }
};

const char *CScaler::m_cszEdgeName[CScaler::SC_NUM_EDGES] = {"source", "destination"};

void CScaler::Initialize(int instance, bool nonblock)
{
    char mutexname[20];

    m_fdScaler = -1;
    m_iInstance = instance;

    snprintf(m_cszNode, SC_MAX_NODENAME, SC_DEV_NODE "%d", SC_NODE(instance));

    m_fdScaler = exynos_v4l2_open(m_cszNode,
                        nonblock ? (O_RDWR | O_NONBLOCK) : (O_RDWR));
    if (m_fdScaler < 0) {
        SC_LOGERR("Failed to open '%s'", m_cszNode);
        return;
    }

    unsigned int cap = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT_MPLANE |
                        V4L2_CAP_VIDEO_CAPTURE_MPLANE;
    if (!exynos_v4l2_querycap(m_fdScaler, cap)) {
        SC_LOGERR("Failed to query capture on '%s'", m_cszNode);
        close(m_fdScaler);
        m_fdScaler = -1;
    } else {
        m_fdValidate = -m_fdScaler;
        memset(&m_Frame, 0, sizeof(m_Frame));

        m_Frame.edge[SC_SRC].fdAcquireFence = -1;
        m_Frame.edge[SC_DST].fdAcquireFence = -1;
    }
}

CScaler::CScaler(int instance, int allow_drm, bool nonblock)
{
    Initialize(instance, nonblock);
    if(Valid()) {
        if (allow_drm)
            SetFlag(SCF_ALLOW_DRM);
        SC_LOGD("Successfully opened '%s'; returned fd %d; drmmode %s",
                m_cszNode, m_fdScaler, allow_drm ? "enabled" : "disabled");
    }
}

CScaler::~CScaler()
{
    if (m_fdScaler >= 0)
        close(m_fdScaler);

    m_fdScaler = -1;
}

int CScaler::Stop()
{
    for (int edge = 0; edge < SC_NUM_EDGES; edge++) {
        if (ResetDevice(static_cast<SC_EDGE>(edge)) != 0) {
            SC_LOGE("Failed to stop Scaler");
            return -1;
        }
    }

    return 0;
}

int CScaler::Start()
{
    int ret;

    ret = SetCtrl();
    if (ret)
        return ret;

    ret = SetFormat();
    if (ret)
        return ret;

    ret = ReqBufs();
    if (ret)
        return ret;

    ret = QBuf();
    if (ret)
        return ret;

    ret = StreamOn();
    if (ret)
        return ret;

    ret = DQBuf();
    if (ret)
        Stop();
    else
        ret = Stop();

    return ret;
}

int CScaler::SetCtrl()
{
    if (!IsSet(SCF_ROTATION_FRESH)) {
        SC_LOGD("Skipping rotation and flip setting due to no change");
        return 0;
    }

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_ROTATE, GetRotDegree()) < 0) {
        SC_LOGERR("Failed V4L2_CID_ROTATE with degree %d", GetRotDegree());
        return -1;
    }

     if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_VFLIP, IsSet(SCF_HFLIP)) < 0) {
        SC_LOGERR("Failed V4L2_CID_VFLIP - %d", IsSet(SCF_VFLIP));
        return -1;
    }

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_HFLIP, IsSet(SCF_VFLIP)) < 0) {
        ALOGE("Failed V4L2_CID_HFLIP - %d", IsSet(SCF_HFLIP));
        return -1;
    }

    SC_LOGD("Successfully set CID_ROTATE(%d), CID_VFLIP(%d) and CID_HFLIP(%d)",
            GetRotDegree(), IsSet(SCF_VFLIP), IsSet(SCF_HFLIP));

    ClearFlag(SCF_ROTATION_FRESH);
    return 0;
}

int CScaler::ResetDevice(SC_EDGE edge)
{
    DQBuf(static_cast<SC_EDGE>(edge));

    if (IsSet(SCF_STREAMING, edge)) {
        if (exynos_v4l2_streamoff(m_fdScaler, m_Frame.edge[edge].type) < 0 ) {
            SC_LOGERR("Failed STREAMOFF for the %s", m_cszEdgeName[edge]);
            return -1;
        }
        ClearFlag(SCF_STREAMING, edge);
    }

    SC_LOGD("VIDIC_STREAMOFF is successful for the %s", m_cszEdgeName[edge]);

    if (IsSet(SCF_REQBUFS, edge)) {
        v4l2_requestbuffers reqbufs;
        memset(&reqbufs, 0, sizeof(reqbufs));
        reqbufs.type = m_Frame.edge[edge].type;
        reqbufs.memory = m_Frame.edge[edge].memory;
        if (exynos_v4l2_reqbufs(m_fdScaler, &reqbufs) < 0 ) {
            SC_LOGERR("Failed to REQBUFS(0) for the %s", m_cszEdgeName[edge]);
            return -1;
        }

        ClearFlag(SCF_REQBUFS, edge);
    }

    SC_LOGD("VIDIC_REQBUFS(0) is successful for the %s", m_cszEdgeName[edge]);

    return 0;
}

int CScaler::SetFormat()
{
    for (int edge = 0; edge < SC_NUM_EDGES; edge++) {
        if (!IsSet(SCF_BUF_FRESH, edge)) {
            SC_LOGD("Skipping S_FMT for the %s since it is already done", m_cszEdgeName[edge]);
            continue; // No new format or control variable is set
        }

        if (ResetDevice(static_cast<SC_EDGE>(edge)) != 0) {
            SC_LOGE("Failed to VIDIOC_S_FMT");
            return -1;
        }

        v4l2_format fmt;
        fmt.type = m_Frame.edge[edge].type;
        fmt.fmt.pix_mp.pixelformat = m_Frame.edge[edge].color_format;
        fmt.fmt.pix_mp.width  = m_Frame.edge[edge].width;
        fmt.fmt.pix_mp.height = m_Frame.edge[edge].height;

        if (exynos_v4l2_s_fmt(m_fdScaler, &fmt) < 0) {
            SC_LOGERR("Failed S_FMT(fmt: %d, w:%d, h:%d) for the %s",
                    fmt.fmt.pix_mp.pixelformat, fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
                    m_cszEdgeName[edge]);
            return -1;
        }

        // returned fmt.fmt.pix_mp.num_planes and fmt.fmt.pix_mp.plane_fmt[i].sizeimage
        m_Frame.edge[edge].out_num_planes = fmt.fmt.pix_mp.num_planes;

        for (int i = 0; i < m_Frame.edge[edge].out_num_planes; i++)
            m_Frame.edge[edge].out_plane_size[i] = fmt.fmt.pix_mp.plane_fmt[i].sizeimage;

        v4l2_crop crop;
        crop.type = m_Frame.edge[edge].type;
        crop.c.left = m_Frame.edge[edge].crop_left;
        crop.c.top = m_Frame.edge[edge].crop_top;
        crop.c.width = m_Frame.edge[edge].crop_width;
        crop.c.height = m_Frame.edge[edge].crop_height;

        if (exynos_v4l2_s_crop(m_fdScaler, &crop) < 0) {
            SC_LOGERR("Failed S_CROP(fmt: %d, l:%d, t:%d, w:%d, h:%d) for the %s",
                    crop.type, crop.c.left, crop.c.top, crop.c.width, crop.c.height,
                    m_cszEdgeName[edge]);
            return -1;
        }

        if (m_Frame.edge[edge].out_num_planes > SC_MAX_PLANES) {
            SC_LOGE("Number of planes exceeds %d", m_Frame.edge[edge].out_num_planes);
            return -1;
        }

        ClearFlag(SCF_BUF_FRESH, edge);
        SC_LOGD("Successfully S_FMT and S_CROP for the %s", m_cszEdgeName[edge]);
    }

    return 0;
}

int CScaler::QBuf()
{
    v4l2_buffer buffer;
    v4l2_plane planes[SC_MAX_PLANES];

    for (int edge = 0; edge < SC_NUM_EDGES; edge++) {
        if (!IsSet(SCF_REQBUFS, edge)) {
            SC_LOGE("Trying to QBUF without REQBUFS for %s is not allowed",
                    m_cszEdgeName[edge]);
            return -1;
        }

        DQBuf(static_cast<SC_EDGE>(edge));

        memset(&buffer, 0, sizeof(buffer));
        memset(&planes, 0, sizeof(planes));

        buffer.type   = m_Frame.edge[edge].type;
        buffer.memory = m_Frame.edge[edge].memory;
        buffer.index  = 0;
        buffer.length = m_Frame.edge[edge].out_num_planes;

        buffer.m.planes = planes;
        for (unsigned long i = 0; i < buffer.length; i++) {
            planes[i].length = m_Frame.edge[edge].out_plane_size[i];
            if (V4L2_TYPE_IS_OUTPUT(buffer.type))
                planes[i].bytesused = planes[i].length;
            if (buffer.memory == V4L2_MEMORY_DMABUF)
                planes[i].m.fd = reinterpret_cast<int>(m_Frame.edge[edge].addr[i]);
            else
                planes[i].m.userptr = reinterpret_cast<unsigned long>(m_Frame.edge[edge].addr[i]);
        }


        if (exynos_v4l2_qbuf(m_fdScaler, &buffer) < 0) {
            SC_LOGERR("Failed to QBUF for the %s", m_cszEdgeName[edge]);
            return -1;
        }

        SetFlag(SCF_QBUF, edge);

        SC_LOGD("Successfully QBUF for the %s", m_cszEdgeName[edge]);
    }

    return 0;
}

int CScaler::QBuf(int pfdReleaseFence[SC_NUM_EDGES])
{

    v4l2_buffer buffer;
    v4l2_plane planes[SC_MAX_PLANES];

    for (int edge = 0; edge < SC_NUM_EDGES; edge++) {
        if (!IsSet(SCF_REQBUFS, edge)) {
            SC_LOGE("Trying to QBUF without REQBUFS for %s is not allowed",
                    m_cszEdgeName[edge]);
            return -1;
        }

        DQBuf(static_cast<SC_EDGE>(edge));

        memset(&buffer, 0, sizeof(buffer));
        memset(&planes, 0, sizeof(planes));

        buffer.type   = m_Frame.edge[edge].type;
        buffer.memory = m_Frame.edge[edge].memory;
        buffer.index  = 0;
        buffer.length = m_Frame.edge[edge].out_num_planes;
        buffer.flags    = V4L2_BUF_FLAG_USE_SYNC;
        buffer.reserved = m_Frame.edge[edge].fdAcquireFence;

        buffer.m.planes = planes;
        for (unsigned long i = 0; i < buffer.length; i++) {
            planes[i].length = m_Frame.edge[edge].out_plane_size[i];
            if (V4L2_TYPE_IS_OUTPUT(buffer.type))
                planes[i].bytesused = planes[i].length;
            if (buffer.memory == V4L2_MEMORY_DMABUF)
                planes[i].m.fd = reinterpret_cast<int>(m_Frame.edge[edge].addr[i]);
            else
                planes[i].m.userptr = reinterpret_cast<unsigned long>(m_Frame.edge[edge].addr[i]);
        }


        if (exynos_v4l2_qbuf(m_fdScaler, &buffer) < 0) {
            SC_LOGERR("Failed to QBUF for the %s", m_cszEdgeName[edge]);
            return -1;
        }

        SetFlag(SCF_QBUF, edge);

        if (m_Frame.edge[edge].fdAcquireFence >= 0) {
            close(m_Frame.edge[edge].fdAcquireFence);
            m_Frame.edge[edge].fdAcquireFence = -1;
        }

        pfdReleaseFence[edge] = static_cast<int>(buffer.reserved);
        SC_LOGD("Successfully QBUF for the %s", m_cszEdgeName[edge]);
    }

    return 0;
}

int CScaler::ReqBufs()
{
    v4l2_requestbuffers reqbufs;

    for (int edge = 0; edge < SC_NUM_EDGES; edge++) {
        if (IsSet(SCF_REQBUFS, edge)) {
            SC_LOGD("Skipping REQBUFS for the %s since it is already done", m_cszEdgeName[edge]);
            continue;
        }

        memset(&reqbufs, 0, sizeof(reqbufs));

        reqbufs.type    = m_Frame.edge[edge].type;
        reqbufs.memory  = m_Frame.edge[edge].memory;
        reqbufs.count   = 1;

        if (exynos_v4l2_reqbufs(m_fdScaler, &reqbufs) < 0) {
            SC_LOGERR("Failed to REQBUFS for the %s", m_cszEdgeName[edge]);
            return -1;
        }

        SetFlag(SCF_REQBUFS, edge);

        SC_LOGD("Successfully REQBUFS for the %s", m_cszEdgeName[edge]);
    }
    return 0;
}

void CScaler::SetImgFormat(
        SC_EDGE edge,
        unsigned int width,
        unsigned int height,
        unsigned int crop_left,
        unsigned int crop_top,
        unsigned int crop_width,
        unsigned int crop_height,
        unsigned int v4l2_colorformat,
        unsigned int cacheable,
        unsigned int mode_drm,
        unsigned int premultiplied)
{
    m_Frame.edge[edge].type = (edge == SC_SRC) ? SC_SRC_BUFTYPE : SC_DST_BUFTYPE;

    m_Frame.edge[edge].color_format = v4l2_colorformat;
    m_Frame.edge[edge].width = width;
    m_Frame.edge[edge].height = height;
    m_Frame.edge[edge].crop_left = crop_left;
    m_Frame.edge[edge].crop_top = crop_top;
    m_Frame.edge[edge].crop_width = crop_width;
    m_Frame.edge[edge].crop_height = crop_height;
    if (premultiplied)
        SetFlag(SCF_PREMULTIPLIED, edge);
    else
        ClearFlag(SCF_PREMULTIPLIED, edge);

    if (cacheable)
        SetFlag(SCF_CACHEABLE, edge);
    else
        ClearFlag(SCF_CACHEABLE, edge);

    if (mode_drm)
        SetFlag(SCF_DRM, edge);
    else
        ClearFlag(SCF_DRM, edge);

    SetFlag(SCF_BUF_FRESH, edge);
}

bool CScaler::SetRotate(int rot, int flip_h, int flip_v)
{
    if ((rot % 90) != 0) {
        SC_LOGE("Rotation of %d degree is not supported", rot);
        return false;
    }

    SetRotDegree(rot);

    if (flip_h)
        SetFlag(SCF_VFLIP);
    else
        ClearFlag(SCF_VFLIP);

    if (flip_v)
        SetFlag(SCF_HFLIP);
    else
        ClearFlag(SCF_HFLIP);

    SetFlag(SCF_ROTATION_FRESH);

    return true;
}

int CScaler::StreamOn()
{
    for (int edge = 0; edge < SC_NUM_EDGES; edge++) {
        if (!IsSet(SCF_REQBUFS, edge)) {
            SC_LOGE("Trying to STREAMON without REQBUFS for %s is not allowed",
                    m_cszEdgeName[edge]);
            return -1;
        }

        if (!IsSet(SCF_STREAMING, edge)) {
            if (exynos_v4l2_streamon(m_fdScaler, m_Frame.edge[edge].type) < 0 ) {
                SC_LOGERR("Failed StreamOn for the %s", m_cszEdgeName[edge]);
                return errno;
            }

            SetFlag(SCF_STREAMING, edge);

            SC_LOGD("Successfully VIDIOC_STREAMON for the %s", m_cszEdgeName[edge]);
        }
    }

    return 0;
}

int CScaler::DQBuf(SC_EDGE edge)
{
    if (!IsSet(SCF_QBUF, edge))
        return 0;

    v4l2_buffer buffer;
    v4l2_plane plane[SC_NUM_OF_PLANES];

    memset(&buffer, 0, sizeof(buffer));

    buffer.type = m_Frame.edge[edge].type;
    buffer.memory = m_Frame.edge[edge].memory;

    if (V4L2_TYPE_IS_MULTIPLANAR(buffer.type)) {
        memset(plane, 0, sizeof(plane));

        buffer.length = m_Frame.edge[edge].out_num_planes;
        buffer.m.planes = plane;
    }

    if (exynos_v4l2_dqbuf(m_fdScaler, &buffer) < 0 ) {
        SC_LOGERR("Failed to DQBuf the %s", m_cszEdgeName[edge]);
        return -1;
    }

    if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
        SC_LOGE("Error occurred while processing streaming data");
        return -1;
    }

    ClearFlag(SCF_QBUF, edge);

    SC_LOGD("Successfully VIDIOC_DQBUF for the %s", m_cszEdgeName[edge]);

    return 0;
}

int CScaler::DQBuf()
{
    for (int edge = 0; edge < SC_NUM_EDGES; edge++) {
        if (DQBuf(static_cast<SC_EDGE>(edge)))
            return -1;
    }
    return 0;
}
