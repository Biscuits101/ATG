[EntityEditorProps(category: "GameScripted/ScriptWizard", description: "ScriptWizard generated script file.")]
class SCR_MarkerComponentClass : ScriptComponentClass
{
}

class SCR_MarkerComponent : ScriptComponent
{
	[Attribute("{F7E8D4834A3AFF2F}UI/Imagesets/Conflict/conflict-icons-bw.imageset")]
	protected ResourceName m_sBuildingIconImageset;
	
	protected ImageWidget m_wWaypoint;
	protected RichTextWidget m_wWaypointDistance;

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
	}
	
	void OnPlayerSpawned(int playerId, IEntity controlledEntity)
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

	//------------------------------------------------------------------------------------------------
	void SCR_MarkerComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
	}

	//------------------------------------------------------------------------------------------------
	void ~SCR_MarkerComponent()
	{
	}

}
