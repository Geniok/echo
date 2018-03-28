#include "NodeTree.h"
#include "engine/core/camera/CameraMain.h"

namespace Echo
{
	__ImplementSingleton(NodeTree);

	// ���캯��
	NodeTree::NodeTree()
		: m_3dCamera(NULL)
		, m_2dCamera(NULL)
		, m_uiCamera(NULL)
		, m_shadowCamera(NULL)
		, m_invisibleRoot(nullptr)
	{
		__ConstructSingleton;
	}

	NodeTree::~NodeTree()
	{
		__DestructSingleton;
	}

	// ��ʼ��
	bool NodeTree::init()
	{
		// create main camera
		m_3dCamera = EchoNew(CameraMain(Camera::PM_PERSPECTIVE));
		m_shadowCamera = EchoNew(CameraShadow);

		Vector3 vCamPos(0, 100, 100);
		Vector3 vCamDir = Vector3::ZERO - vCamPos;
		vCamDir.normalize();

		m_3dCamera->setPosition(vCamPos);
		m_3dCamera->setDirection(vCamDir);

		// create 2D camera
		Vector3 vCam2DPos(0, 0, 1);
		Vector3 vCam2DDir = -Vector3::UNIT_Z;
		m_2dCamera = EchoNew(Camera(Camera::PM_ORTHO));

		m_2dCamera->setPosition(vCam2DPos);
		m_2dCamera->setDirection(vCam2DDir);
		m_2dCamera->setNearClip(0.1f);
		m_2dCamera->setFarClip(100.0f);
		m_2dCamera->update();

		update(0.001f);

		return true;
	}

	// ����
	void NodeTree::destroy()
	{
		EchoSafeDelete(m_3dCamera, Camera);
		EchoSafeDelete(m_2dCamera, Camera);
		EchoSafeDelete(m_shadowCamera, CameraShadow);
	}

	// ���� 
	void NodeTree::update(float elapsedTime)
	{
		if (m_invisibleRoot)
		{
			m_invisibleRoot->update(true);
		}
	}
}