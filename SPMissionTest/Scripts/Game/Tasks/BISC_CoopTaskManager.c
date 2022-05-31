[EntityEditorProps(category: "GameScripted/Biscuits", description: "COOP Friendly Task Manager.")]
class BISC_CoopTaskManagerClass : SCR_BaseTaskManagerClass
{
}

class BISC_CoopTaskManager : SCR_BaseTaskManager
{
	[Attribute("US", UIWidgets.EditBox, desc: "Faction to assign to tasks.", category: "Task Manager")]
	protected FactionKey m_sCOOPFaction;

	[Attribute("", UIWidgets.Auto, desc: "Entity names of tasks in order - assigned to connecting players automatically.", category: "Task Manager")]
	protected ref array<string> m_aTaskEntityNames;

	[Attribute("0", UIWidgets.CheckBox, desc: "Enables additional console output.", category: "Task Manager")]
	protected bool m_bVerbose;


	// For detecting authority / proxy at runtime
	protected RplComponent m_pRplComponent;

	// For UI prompts and is generally required so check
	protected SCR_UITaskManagerComponent m_pUITaskManagerComponent;

	// Actual target faction
	protected Faction m_pTargetFaction;

	// Array of actual task entities
	protected ref array<SCR_BaseTask> m_aTaskEntities;

	// For optional waypoint markers (add BISC_WaypointMarkerComponent)
	protected BISC_WaypointMarkerComponent m_pWaypointMarkerComponent;

	// Synchronised Task Id
	[RplProp(onRplName: "OnWaypointUpdated", condition: RplCondition.None)]
	protected int m_iCurrentTaskId = -1;


	protected override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (!GetGame().InPlayMode())
			return;

		// Find RplComponent or abort
		m_pRplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!m_pRplComponent)
		{
			Print("Task Manager is missing RplComponent!", LogLevel.ERROR);
			return;
		}

		// Find SCR_UITaskManagerComponent or abort
		m_pUITaskManagerComponent = SCR_UITaskManagerComponent.Cast(owner.FindComponent(SCR_UITaskManagerComponent));
		if (!m_pUITaskManagerComponent)
		{
			Print("Task Manager is missing SCR_UITaskManagerComponent!", LogLevel.ERROR);
			return;
		}

		// Server setup
		if (m_pRplComponent.IsMaster())
			ServerInit();

		// Setup for optional waypoint markers
		if (RplSession.Mode() != RplMode.Dedicated)
			m_pWaypointMarkerComponent = BISC_WaypointMarkerComponent.Cast(owner.FindComponent(BISC_WaypointMarkerComponent));
	}


	protected void ServerInit()
	{
		// Find faction from provided key or abort
		m_pTargetFaction = GetGame().GetFactionManager().GetFactionByKey(m_sCOOPFaction);
		if (!m_pTargetFaction)
		{
			Print("Task Manager could not find the Faction belonging to FACTION_KEY: " + m_sCOOPFaction, LogLevel.ERROR);
			return;
		}

		// Find the chosen task entities and fill/order them into the array
		OrganiseTasks(m_bVerbose);
	}


	override void OnTaskUpdate(SCR_BaseTask task)
	{
		super.OnTaskUpdate(task);

		if (m_bVerbose)
			Print("TASK '" + task.GetName() + "' UPDATED: " + SCR_Enum.GetEnumName(SCR_TaskState, task.GetTaskState()), LogLevel.NORMAL);

		// If not master authority
		if (!m_RplComponent || !m_RplComponent.IsMaster())
			return;

		// If task was finished, try assign the next
		if (task.GetTaskState() == SCR_TaskState.FINISHED)
			TryAssignNextTask();
	}


	////////////////////////////////////////////////////////////////
	///// CALLED ON EVERY MACHINE FOR EVERY PLAYER REGISTERED //////
	// INCLUDING WHEN JIP, PREVIOUS REGISTERED PLAYERS ARE CALLED //
	////////////////////////////////////////////////////////////////
	override void OnPlayerRegistered(int registeredPlayerID)
	{
		super.OnPlayerRegistered(registeredPlayerID);

		if (m_bVerbose)
			Print(SCR_Enum.GetEnumName(RplRole, m_pRplComponent.Role()) + " : Player " + registeredPlayerID.ToString() + " registered!", LogLevel.NORMAL);

		// Authority only
		if (m_pRplComponent.IsMaster())
			TryAssignNextTask(registeredPlayerID);

		bool isPlayerInQuestion = GetGame().GetPlayerController().GetPlayerId() == registeredPlayerID;

		// If im the registering player, show my task HUD in a bit
		if (isPlayerInQuestion)
			GetGame().GetCallqueue().CallLater(ShowCurrentTask, 3000, false);
	}


	void TryAssignNextTask(int joinInProgressId = -1)
	{
		// Find first unfinished task
		foreach (SCR_BaseTask aTask : m_aTaskEntities)
		{
			if (!aTask)
				continue;

			if (aTask.GetTaskState() == SCR_TaskState.FINISHED)
				continue;

			// Setting its faction for all players, not done till now for journal visibility
			super.SetTaskFaction(aTask, m_pTargetFaction);

			// Specific player who just registered
			if (joinInProgressId != -1)
			{
				SCR_BaseTaskExecutor playerTaskExecutor = SCR_BaseTaskExecutor.GetTaskExecutorByID(joinInProgressId);

				if (playerTaskExecutor)
					super.AssignTask(aTask, playerTaskExecutor, false);
			}
			else
			{
				// Get all connected player ids
				array<int> playerIds = new array<int>();
				GetGame().GetPlayerManager().GetPlayers(playerIds);

				// Force assign the task to each connected player
				foreach (int playerId : playerIds)
				{
					SCR_BaseTaskExecutor playerTaskExecutor = SCR_BaseTaskExecutor.GetTaskExecutorByID(playerId);

					if (playerTaskExecutor)
						super.AssignTask(aTask, playerTaskExecutor, false);
				}

				// Toggle the task hint on clients HUDs
				Rpc(RpcDo_ShowCurrentTask);

				// And for self if needed
				if (RplSession.Mode() != RplMode.Dedicated)
					ShowCurrentTask();
			}

			// Only sets/bumps on actual changes to cut down on net traffic
			if (m_iCurrentTaskId != aTask.GetTaskID())
			{
				// Update and inform clients
				m_iCurrentTaskId = aTask.GetTaskID();
				Replication.BumpMe();

				// Also do self if not dedicated server
				if (RplSession.Mode() != RplMode.Dedicated)
					OnWaypointUpdated();
			}

			// We have already tried to assign a task, so stop searching
			break;
		}
	}


	protected void OrganiseTasks(bool debugText = false)
	{
		// Initialise task entities array
		m_aTaskEntities = new array<SCR_BaseTask>();

		// SCR_BaseTaskManager has already collected all tasks in the scene
		array<SCR_BaseTask> allTaskEntities = new array<SCR_BaseTask>();
		super.GetTasks(allTaskEntities);

		// Pre-emptively resize the array for inserting tasks in order
		m_aTaskEntities.Resize(m_aTaskEntityNames.Count());

		// Now to order them
		foreach (SCR_BaseTask taskEntity : allTaskEntities)
		{
			string taskEntityName = taskEntity.GetName();
			int taskIndex = m_aTaskEntityNames.Find(taskEntityName);

			// Task was not mentioned in TaskEntityNames attribute
			if (taskIndex == -1)
				continue;

			// Place in the attribute order
			m_aTaskEntities.Set(taskIndex, taskEntity);
		}

		// DEBUG
		if (debugText)
		{
			LogLevel level = LogLevel.NORMAL;

			Print("ALL TASKS (UNORDERED):", level);
			foreach (SCR_BaseTask aTask : allTaskEntities)
				if (aTask)
					Print(aTask.GetTaskID().ToString() + ": " + aTask.GetName() + " - " + aTask.GetTitle(), level);

			Print("CHOSEN TASKS (ORDERED):", level);
			foreach (SCR_BaseTask aTask : m_aTaskEntities)
				if (aTask)
					Print(aTask.GetTaskID().ToString() + ": " + aTask.GetName() + " - " + aTask.GetTitle(), level);
		}
	}


	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ShowCurrentTask()
	{
		ShowCurrentTask();
	}

	protected void ShowCurrentTask()
	{
		if (m_pUITaskManagerComponent)
			m_pUITaskManagerComponent.ToggleCurrentTask(true);
	}

	protected void OnWaypointUpdated()
	{
		if (m_pWaypointMarkerComponent)
			m_pWaypointMarkerComponent.SetWaypoint(m_iCurrentTaskId);
	}
}