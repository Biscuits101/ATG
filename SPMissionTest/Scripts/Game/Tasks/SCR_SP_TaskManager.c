[EntityEditorProps(category: "GameScripted/ScriptWizard", description: "ScriptWizard generated script file.")]
class SCR_SP_TaskManagerClass : SCR_BaseTaskManagerClass
{
}

class SCR_SP_TaskManager : SCR_BaseTaskManager
{

	[Attribute("", UIWidgets.Auto, "Entity names of intial tasks - assigned to connecting players automatically.", category: "SP TaskManager")]
	protected ref array<string> m_aInitialTaskNames;

	[Attribute("US", UIWidgets.EditBox, "Faction key to assign to tasks.", category: "SP TaskManager")]
	protected FactionKey m_sAssignedFaction;
	
	//! Runtime instances for tasks created from initial task names
	protected ref array<SCR_BaseTask> m_aInitialTasks;
	
	// Task Executors appear to be player owned task managers
	protected ref array<SCR_BaseTaskExecutor> m_aTaskExecutors;

	protected RplComponent m_pRplComponent;
	
	protected Faction m_pTargetFaction;
	
	protected SCR_UITaskManagerComponent m_pTaskManagerUIComponent;
	
	[Attribute("{F7E8D4834A3AFF2F}UI/Imagesets/Conflict/conflict-icons-bw.imageset")]
	protected ResourceName m_sBuildingIconImageset;
	
	[Attribute("true", UIWidgets.CheckBox, "Show waypoints", category: "SP Waypoints")]
	protected bool m_bShowWaypoint;
	
	protected float m_fWaypointHeight = 1.0;
	
	protected ImageWidget m_wWaypoint;
	protected RichTextWidget m_wWaypointDistance;
	protected SCR_BaseTask m_CurrentWaypoint;
	
	//------------------------------------------------------------------------------------------------
	protected override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		// Do not initialize these tasks out of runtime
		if (!GetGame().InPlayMode())
			return;

		m_aInitialTasks = {};
		m_aTaskExecutors = {};
		
		BaseWorld world = owner.GetWorld();
		foreach (string taskName : m_aInitialTaskNames)
		{
			IEntity entity = world.FindEntityByName(taskName);
			SCR_BaseTask task = SCR_BaseTask.Cast(entity);
			if (!task)
				continue;
			
			m_aInitialTasks.Insert(task);
		}
		
		m_pTaskManagerUIComponent = SCR_UITaskManagerComponent.Cast(owner.FindComponent(SCR_UITaskManagerComponent));
		if (m_pTaskManagerUIComponent)
			Print("Missing SCR_UITaskManagerComponent", LogLevel.ERROR);

		m_pRplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!m_pRplComponent)
		{
			Print("SCR_CoopTaskManager is missing m_pRplComponent!", LogLevel.ERROR);
			return;
		}

		// Authority only
		if (!m_pRplComponent.IsMaster())
			return;

		m_pTargetFaction = GetGame().GetFactionManager().GetFactionByKey(m_sAssignedFaction);
		if (!m_pTargetFaction)
		{
			Print("SCR_CoopTaskManager is missing Faction to assign tasks to!", LogLevel.ERROR);
			return;
		}
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
	}
	
	// If a task is completed move on to the next task
	protected override void OnTaskUpdate(SCR_BaseTask task)
	{
		super.OnTaskUpdate(task);
		
		if (task.GetTaskState() == SCR_TaskState.FINISHED)
		{
			foreach (SCR_BaseTaskExecutor taskExecutor : m_aTaskExecutors)
			{
				AssignNextTask(taskExecutor);
			}
		}
	}

	// On player join, register their TaskExecutor and give them the current task
	protected override void OnPlayerRegistered(int registeredPlayerID)
	{
		super.OnPlayerRegistered(registeredPlayerID);

		SCR_BaseTaskExecutor taskExecutor = SCR_BaseTaskExecutor.GetTaskExecutorByID(registeredPlayerID);
		if (!taskExecutor) return;
		
		m_aTaskExecutors.Insert(taskExecutor);
		
		AssignNextTask(taskExecutor);
		
		// Waypoint system from SP Tutorial
		WaypointSystemInit();
	}
	
	// On player leave, unregister their TaskExecutor, the base class handles task unassignment
	protected override void OnPlayerDisconnected(int id)
	{
		super.OnPlayerDisconnected(id);
		
		SCR_BaseTaskExecutor taskExecutor = SCR_BaseTaskExecutor.GetTaskExecutorByID(id);
		if (!taskExecutor) return;
		
		m_aTaskExecutors.RemoveItem(taskExecutor);
	}
	
	// Assign the next task in the list for a given TaskExecutor
	protected void AssignNextTask(SCR_BaseTaskExecutor taskExecutor)
	{
		foreach (SCR_BaseTask task : m_aInitialTasks)
		{
			if (!task)
				continue;

			if (task.GetTaskState() == SCR_TaskState.FINISHED)
				continue;
			
			// Setting the target faction invokes things so only do this when needed
			// IE due to completion of previous task and not a player joining the current task
			if (task.GetTargetFaction() != m_pTargetFaction)
				task.SetTargetFaction(m_pTargetFaction);
			
			// Assign the first task that is incomplete
			AssignTask(task, taskExecutor, false);
			
			// TESTING
			m_pTaskManagerUIComponent.ToggleCurrentTask(true);
			
			m_CurrentWaypoint = task;
			break;
		}
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		// Render waypoint if allowed
		if (m_wWaypoint)
		{
			if (m_CurrentWaypoint && m_bShowWaypoint)
			{
				IEntity player = EntityUtils.GetPlayer();
				if (!player) return;
				
				vector WPPos = m_CurrentWaypoint.GetOrigin();
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