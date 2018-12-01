#include "TimelinePanel.h"
#include "engine/core/editor/editor.h"
#include "engine/core/editor/qt/QUiLoader.h"
#include "engine/core/editor/qt/QSplitter.h"
#include "engine/core/editor/qt/QToolButton.h"
#include "engine/core/editor/qt/QComboBox.h"
#include "engine/core/editor/qt/QMenu.h"
#include "engine/core/editor/qt/QTreeWidget.h"
#include "engine/core/editor/qt/QTreeWidgetItem.h"
#include "engine/core/editor/qt/QHeaderView.h"
#include "engine/core/base/class_method_bind.h"
#include "engine/core/util/PathUtil.h"
#include "../../anim_timeline.h"

namespace Echo
{
#ifdef ECHO_EDITOR_MODE
	TimelinePanel::TimelinePanel(Object* obj)
		: m_addObjectMenu(nullptr)
		, m_nodeTreeWidgetWidth(0)
	{
		m_timeline = ECHO_DOWN_CAST<Timeline*>(obj);

		m_ui = qLoadUi("engine/modules/anim/editor/TimelinePanel/TimelinePanel.ui");

		QWidget* splitter = qFindChild(m_ui, "m_splitter");
		if (splitter)
		{
			qSplitterSetStretchFactor(splitter, 0, 0);
			qSplitterSetStretchFactor(splitter, 1, 1);
		}

		// Top toolbuttons icons
		qToolButtonSetIcon(qFindChild(m_ui, "AddNode"), "engine/modules/anim/editor/icon/add.png");
		qToolButtonSetIcon(qFindChild(m_ui, "Play"), "engine/modules/anim/editor/icon/play.png");
		qToolButtonSetIcon(qFindChild(m_ui, "Stop"), "engine/modules/anim/editor/icon/stop.png");
		qToolButtonSetIcon(qFindChild(m_ui, "Restart"), "engine/modules/anim/editor/icon/replay.png");

		// set fixed width of add toolbutton
		qToolButtonSetIconSize(qFindChild(m_ui, "AddNode"), 24, 24);

		// set toolbuttons icons
		qToolButtonSetIcon( qFindChild(m_ui, "NewClip"), "engine/modules/anim/editor/icon/new.png");
		qToolButtonSetIcon( qFindChild(m_ui, "DuplicateClip"), "engine/modules/anim/editor/icon/duplicate.png");
		qToolButtonSetIcon( qFindChild(m_ui, "DeleteClip"), "engine/modules/anim/editor/icon/delete.png");

		// connect signal slots
		qConnect(qFindChild(m_ui, "NewClip"), QSIGNAL(clicked()), this, createMethodBind(&TimelinePanel::onNewClip));
		qConnect(qFindChild(m_ui, "DuplicateClip"), QSIGNAL(clicked()), this, createMethodBind(&TimelinePanel::onDuplicateClip));
		qConnect(qFindChild(m_ui, "DeleteClip"), QSIGNAL(clicked()), this, createMethodBind(&TimelinePanel::onDeleteClip));
		qConnect(qFindChild(m_ui, "m_clips"), QSIGNAL(editTextChanged(const QString &)), this, createMethodBind(&TimelinePanel::onRenameClip));
		qConnect(qFindChild(m_ui, "m_clips"), QSIGNAL(currentIndexChanged(int)), this, createMethodBind(&TimelinePanel::onCurrentEditAnimChanged));
		qConnect(qFindChild(m_ui, "AddNode"), QSIGNAL(clicked()), this, createMethodBind(&TimelinePanel::onAddObject));
		qConnect(qFindChild(m_ui, "m_nodeTreeWidget"), QSIGNAL(itemClicked(QTreeWidgetItem*, int)), this, createMethodBind(&TimelinePanel::onAddProperty));
		qConnect(qFindChild(m_ui, "Play"), QSIGNAL(clicked()), this, createMethodBind(&TimelinePanel::onPlayAnim));
		qConnect(qFindChild(m_ui, "Stop"), QSIGNAL(clicked()), this, createMethodBind(&TimelinePanel::onStopAnim));
		qConnect(qFindChild(m_ui, "Restart"), QSIGNAL(clicked()), this, createMethodBind(&TimelinePanel::onRestartAnim));

		// update display
		syncClipListDataToEditor();
	}

	void TimelinePanel::onNewClip()
	{
		if (m_timeline)
		{
			AnimClip* animClip = EchoNew(AnimClip);
			animClip->m_name = getNewClipName();
			m_timeline->addClip(animClip);

			syncClipListDataToEditor();

			// set current edit anim clip
			setCurrentEditAnim(animClip->m_name.c_str());
		}
	}

	void TimelinePanel::update()
	{
		onNodeTreeWidgetSizeChanged();
	}

	void TimelinePanel::onDuplicateClip()
	{
		String currentAnim = qComboBoxCurrentText(qFindChild(m_ui, "m_clips"));
		if (!currentAnim.empty() && m_timeline)
		{
			AnimClip* animClip = m_timeline->getClip(currentAnim.c_str());
			AnimClip* animClipDuplicate = animClip->duplicate();
			m_timeline->generateUniqueAnimName(currentAnim + " Duplicate ", animClipDuplicate->m_name);
			m_timeline->addClip(animClipDuplicate);

			syncClipListDataToEditor();

			// set current edit anim clip
			setCurrentEditAnim(animClipDuplicate->m_name.c_str());
		}
	}

	void TimelinePanel::onDeleteClip()
	{
		String currentAnim = qComboBoxCurrentText(qFindChild(m_ui, "m_clips"));
		if (!currentAnim.empty() && m_timeline)
		{
			m_timeline->deleteClip(currentAnim.c_str());

			syncClipListDataToEditor();

			if (m_timeline->getClipCount() > 0)
			{
				setCurrentEditAnim(m_timeline->getClip(0)->m_name.c_str());
			}
		}
	}

	void TimelinePanel::onRenameClip()
	{
		String currentAnimName = qComboBoxCurrentText(qFindChild(m_ui, "m_clips"));
		if (!currentAnimName.empty() && m_timeline)
		{
			int currentIndex = qComboBoxCurrentIndex(qFindChild(m_ui, "m_clips"));
			m_timeline->renameClip(currentIndex, currentAnimName.c_str());

			qComboBoxSetItemText(qFindChild(m_ui, "m_clips"), currentIndex, currentAnimName.c_str());
		}
	}

	void TimelinePanel::onAddObject()
	{
		if (!m_addObjectMenu)
		{
			m_addObjectMenu = qMenuNew(m_ui);

			qMenuAddAction(m_addObjectMenu, qFindChildAction(m_ui, "m_actionAddNode"));
			qMenuAddAction(m_addObjectMenu, qFindChildAction(m_ui, "m_actionAddSetting"));
			qMenuAddAction(m_addObjectMenu, qFindChildAction(m_ui, "m_actionAddResource"));

			qConnect(qFindChildAction(m_ui, "m_actionAddNode"), QSIGNAL(triggered()), this, createMethodBind(&TimelinePanel::onAddNode));
			qConnect(qFindChildAction(m_ui, "m_actionAddSetting"), QSIGNAL(triggered()), this, createMethodBind(&TimelinePanel::onAddSetting));
			qConnect(qFindChildAction(m_ui, "m_actionAddResource"), QSIGNAL(triggered()), this, createMethodBind(&TimelinePanel::onAddResource));
		}

		qMenuExec(m_addObjectMenu);
	}

	void TimelinePanel::onAddNode()
	{
		Echo::String path = Editor::instance()->selectANodeObject();
		if (!path.empty())
		{
			Node* node = m_timeline->getNode(path.c_str());
			if (node)
			{
				String relativePath = node->getNodePathRelativeTo(m_timeline);
				m_timeline->addObject(m_currentEditAnim, Timeline::Node, relativePath);
			}

			syncClipNodeDataToEditor();
		}
	}

	void TimelinePanel::onAddSetting()
	{
		Echo::String path = Editor::instance()->selectASettingObject();
		if (!path.empty())
		{
			m_timeline->addObject(m_currentEditAnim, Timeline::Setting, path);

			syncClipNodeDataToEditor();
		}
	}

	void TimelinePanel::onAddResource()
	{
		Echo::String path = Editor::instance()->selectAResObject();
		if (!path.empty())
		{
			m_timeline->addObject(m_currentEditAnim, Timeline::Resource, path);

			syncClipNodeDataToEditor();
		}
	}

	String TimelinePanel::getNewClipName()
	{
		if (m_timeline)
		{
			Echo::String newName;
			m_timeline->generateUniqueAnimName("Anim ", newName);
			return newName;
		}

		return StringUtil::BLANK;
	}

	// sync clip data to editor
	void TimelinePanel::syncClipListDataToEditor()
	{
		if (m_timeline)
		{
			QWidget* comboBox = qFindChild(m_ui, "m_clips");
			if (comboBox)
			{
				qComboBoxClear(comboBox);

				const StringOption& anims = m_timeline->getAnim();
				for (const String& animName : anims.getOptions())
				{
					qComboBoxAddItem(comboBox, nullptr, animName.c_str());
				}
			}
		}
	}

	void TimelinePanel::syncClipNodeDataToEditor()
	{
		QWidget* nodeTreeWidget = qFindChild(m_ui, "m_nodeTreeWidget");
		if (nodeTreeWidget)
		{
			qTreeWidgetClear(nodeTreeWidget);

			AnimClip* clip = m_timeline->getClip(m_currentEditAnim.c_str());
			if (clip)
			{
				QTreeWidgetItem* rootItem = qTreeWidgetInvisibleRootItem(nodeTreeWidget);
				if (rootItem)
				{
					for (AnimObject* animNode : clip->m_objects)
					{
						const Timeline::ObjectUserData& userData = any_cast<Timeline::ObjectUserData>(animNode->m_userData);
						Node* node = m_timeline->getNode(userData.m_path.c_str());
						QTreeWidgetItem* objetcItem = qTreeWidgetItemNew();
						qTreeWidgetItemSetText(objetcItem, 0, userData.m_path.c_str());
						qTreeWidgetItemSetIcon(objetcItem, 0, Editor::instance()->getNodeIcon(node).c_str());
						qTreeWidgetItemSetIcon(objetcItem, 1, "engine/modules/anim/editor/icon/add.png");
						qTreeWidgetItemAddChild(rootItem, objetcItem);

						for (AnimProperty* property : animNode->m_properties)
						{
							const String& propertyName = any_cast<String>(property->m_userData);
							QTreeWidgetItem* propertyItem = qTreeWidgetItemNew();
							qTreeWidgetItemSetText(propertyItem, 0, propertyName.c_str());
							qTreeWidgetItemSetExpanded(objetcItem, true);
							qTreeWidgetItemAddChild(objetcItem, propertyItem);
						}
					}
				}
			}
		}
	}

	void TimelinePanel::addNodePropertyToEditor()
	{

	}

	void TimelinePanel::setCurrentEditAnim(const char* animName)
	{
		QWidget* comboBox = qFindChild(m_ui, "m_clips");
		if (comboBox)
		{
			int index = m_timeline->getClipIndex(animName);
			qComboBoxSetCurrentIndex( comboBox, index);

			m_currentEditAnim = animName;

			// sync clip node data to editor
			syncClipNodeDataToEditor();
		}
	}

	void TimelinePanel::onAddProperty()
	{
		QTreeWidgetItem* item = qTreeWidgetCurrentItem(qFindChild(m_ui, "m_nodeTreeWidget"));
		int column = qTreeWidgetCurrentColumn(qFindChild(m_ui, "m_nodeTreeWidget"));
		if (column == 1)
		{
			String text = qTreeWidgetItemText(item, 0);
			Node* node = m_timeline->getNode(text.c_str());
			if (node)
			{
				String propertyName = Editor::instance()->selectAProperty( node);
				if (!propertyName.empty())
				{
					m_timeline->addProperty(m_currentEditAnim, node, propertyName);

					// addNodePropertyToEditor;
					QTreeWidgetItem* propertyItem = qTreeWidgetItemNew();
					qTreeWidgetItemSetText(propertyItem, 0, propertyName.c_str());
					qTreeWidgetItemSetExpanded(item, true);
					qTreeWidgetItemAddChild(item, propertyItem);
				}
			}
		}
	}

	void TimelinePanel::onPlayAnim()
	{
		if (m_timeline->getPlayState() != Timeline::PlayState::Playing)
		{
			m_timeline->play(m_currentEditAnim.c_str());

			qToolButtonSetIcon(qFindChild(m_ui, "Play"), "engine/modules/anim/editor/icon/pause.png");
		}
		else
		{
			m_timeline->pause();

			qToolButtonSetIcon(qFindChild(m_ui, "Play"), "engine/modules/anim/editor/icon/play.png");
		}
	}

	void TimelinePanel::onStopAnim()
	{
		m_timeline->stop();

		// recover play button icon to "play.png"
		qToolButtonSetIcon(qFindChild(m_ui, "Play"), "engine/modules/anim/editor/icon/play.png");
	}

	void TimelinePanel::onRestartAnim()
	{
		onStopAnim();
		onPlayAnim();
	}

	void TimelinePanel::onNodeTreeWidgetSizeChanged()
	{
		QWidget* nodeTreeWidget = qFindChild(m_ui, "m_nodeTreeWidget");
		if (nodeTreeWidget)
		{
			int curWidth = qTreeWidgetWidth(nodeTreeWidget);
			if (m_nodeTreeWidgetWidth != curWidth)
			{
				QWidget* header = qTreeWidgetHeader(nodeTreeWidget);

				qHeaderViewResizeSection(header, 1, 30);
				qHeaderViewSetSectionResizeMode(header, 1, QHeaderViewResizeMode::Fixed);
				qHeaderViewResizeSection(header, 0, curWidth - 30);
				m_nodeTreeWidgetWidth = qTreeWidgetWidth(nodeTreeWidget);
			}
		}
	}

	// current edit anim changed
	void TimelinePanel::onCurrentEditAnimChanged()
	{
		String currentText = qComboBoxCurrentText(qFindChild(m_ui, "m_clips"));
		setCurrentEditAnim( currentText.c_str());

		onStopAnim();
	}
#endif
}