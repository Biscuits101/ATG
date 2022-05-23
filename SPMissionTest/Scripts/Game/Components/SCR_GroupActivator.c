[EntityEditorProps(category: "GameScripted/Biscuits", description: "ScriptWizard generated script file.")]
class SCR_GroupActivatorClass : ScriptComponentClass
{
}

class SCR_GroupActivator : ScriptComponent
{
	[Attribute("", UIWidgets.EditBox, desc: "Trigger to attach to.", category: "Activation")]
	protected string m_sTriggerName;
	
	[Attribute("1", UIWidgets.CheckBox, desc: "Stops multiple activations.", category: "Activation")]
	protected bool m_bActivateOnlyOnce;
	
	protected SCR_BaseTriggerEntity m_pActivationTrigger;
	protected SCR_AIGroup m_pAIGroup;
	
	// Seems to be the done thing ¯\_(ツ)_/¯
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		// Only on runtime
		if (!GetGame().InPlayMode())
			return;
		
		if (m_sTriggerName.IsEmpty())
		{
			Print("SCR_GroupActivator trigger name is empty, you donut!");
			return;
		};
		
		// TODO: Should check if there is a better way to limit what it can be added to
		m_pAIGroup = SCR_AIGroup.Cast(owner);
		if (!m_pAIGroup)
		{
			Print("SCR_GroupActivator should only be added to an SCR_AIGroup!");
			return;
		}
		
		// Try to get the named trigger
		BaseWorld world = owner.GetWorld();
		IEntity foundEntity = world.FindEntityByName(m_sTriggerName);
		m_pActivationTrigger = SCR_BaseTriggerEntity.Cast(foundEntity);
		
		if (!m_pActivationTrigger)
		{
			Print("SCR_GroupActivator cannot hook to trigger, maybe check the supplied name!");
			return;
		}
		
		// Subscribe to trigger activations
		m_pActivationTrigger.GetOnActivate().Insert(OnTriggerActivate);
	}
	
	// Removes subscription on deletion of the owning entity
	// Groups have a delete when empty checkbox
	override void OnDelete(IEntity owner)
	{
		TriggerUnsubscribe();
	}
	
	// And finally spawn the units in the group
	protected void OnTriggerActivate()
	{
		if (m_pAIGroup)
			m_pAIGroup.SpawnUnits();
		
		if (m_bActivateOnlyOnce)
			TriggerUnsubscribe();
	}
	
	protected void TriggerUnsubscribe()
	{
		if (m_pActivationTrigger)
			m_pActivationTrigger.GetOnActivate().Remove(OnTriggerActivate);
	}
}