[EntityEditorProps(category: "GameScripted/Biscuits", description: "COOP Friendly Task Manager Waypoints.")]
class BISC_WaypointMarkerComponentClass : ScriptComponentClass
{
}

class BISC_WaypointMarkerComponent : ScriptComponent
{
	[Attribute("1.0", UIWidgets.Range, desc: "Height above the task location the waypoint should appear.")]
	protected float m_fWaypointHeight;

	protected ResourceName m_sBuildingIconImageset = "{F7E8D4834A3AFF2F}UI/Imagesets/Conflict/conflict-icons-bw.imageset";
	protected ImageWidget m_wWaypoint;
	protected RichTextWidget m_wWaypointDistance;

	protected SCR_BaseTask m_pCurrentWaypoint;
	protected bool m_bShowWaypoint = false;
	protected bool m_bHasInitilised = false;


	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}

	override void EOnInit(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return;

		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		GetTaskManager().s_OnTaskAssigned.Insert(TaskAssigned);
		GetTaskManager().s_OnTaskUnassigned.Insert(TaskUnassigned);

		GetGame().GetCallqueue().CallLater(WaypointSystemInit, 2500, false);
	}

	protected void TaskAssigned()
	{
		m_bShowWaypoint = true;
	}

	protected void TaskUnassigned()
	{
		m_bShowWaypoint = false;
	}

	void ClearWaypoint()
	{
		m_pCurrentWaypoint = null;
	}

	void SetWaypoint(int taskId)
	{
		m_pCurrentWaypoint = GetTaskManager().GetTask(taskId);
	}

	protected void WaypointSystemInit()
	{
		Widget waypointFrame = GetGame().GetHUDManager().CreateLayout("{4FEF75768BFDA310}UI/layouts/Tutorial/TutorialWaypoint.layout", EHudLayers.BACKGROUND);
		m_wWaypoint = ImageWidget.Cast(waypointFrame.FindAnyWidget("Icon"));
		m_wWaypointDistance = RichTextWidget.Cast(waypointFrame.FindAnyWidget("Distance"));

		if (m_wWaypoint)
		{
			m_wWaypoint.SetOpacity(0);
			m_wWaypoint.LoadImageFromSet(0, m_sBuildingIconImageset, "USSR_Base_Main_Select");
			m_wWaypoint.SetColor(Color.Yellow);
			FrameSlot.SetSize(m_wWaypoint, 64, 64);
		}

		if (m_wWaypointDistance)
		{
			m_wWaypointDistance.SetOpacity(0);
			m_wWaypointDistance.SetColor(Color.Yellow);
		}

		m_bHasInitilised = true;
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_wWaypoint && m_bHasInitilised)
		{
			if (m_pCurrentWaypoint && m_bShowWaypoint)
			{
				IEntity player = EntityUtils.GetPlayer();
				if (!player) return;

				vector WPPos = m_pCurrentWaypoint.GetOrigin();
				WPPos[1] = WPPos[1] + m_fWaypointHeight;
				vector pos = GetGame().GetWorkspace().ProjWorldToScreen(WPPos, GetGame().GetWorld());
				int dist = vector.Distance(player.GetOrigin(), WPPos);

				// Handle off-screen coords
				WorkspaceWidget workspace = GetGame().GetWorkspace();
				int winX = workspace.GetWidth();
				int winY = workspace.GetHeight();
				int posX = workspace.DPIScale(pos[0]);
				int posY = workspace.DPIScale(pos[1]);

				if (posX < 0)
					pos[0] = 0;
				else if (posX > winX)
					pos[0] = workspace.DPIUnscale(winX);

				if (posY < 0)
					pos[1] = 0;
				else if (posY > winY || pos[2] < 0)
					pos[1] = workspace.DPIUnscale(winY);

				FrameSlot.SetPos(m_wWaypoint, pos[0], pos[1]);
				FrameSlot.SetPos(m_wWaypointDistance, pos[0], pos[1]);
				m_wWaypoint.SetOpacity(1);
				m_wWaypointDistance.SetTextFormat("#AR-Tutorial_WaypointUnits_meters", dist);
				m_wWaypointDistance.SetOpacity(1);
			}
			else
			{
				m_wWaypoint.SetOpacity(0);
				m_wWaypointDistance.SetOpacity(0);
			}
		}
	}
}
