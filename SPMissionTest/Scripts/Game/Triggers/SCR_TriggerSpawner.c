[EntityEditorProps(category: "GameScripted/Biscuits", description: "ScriptWizard generated script file.")]
class SCR_TriggerSpawnerClass : ScriptComponentClass
{
}

class SCR_TriggerSpawner : ScriptComponent
{
	[Attribute("{A752DBCB112E9E57}Prefabs/Structures/Signs/Warnings/SignNoStopping_01_pole.et", UIWidgets.EditBox, "Prefab to spawn.", category: "Spawning")]
	protected ResourceName m_rnSpawnPrefab;
	
	[Attribute("1", UIWidgets.CheckBox, "Spawn on first activation only.", category: "Spawning")]
	protected bool m_bOneShot;
	
	[Attribute("", UIWidgets.EditBox, "Message to announce on trigger", category: "Spawning")]
	protected string m_sTriggerMessage;

	protected RplComponent m_pRplComponent;
	protected vector m_vSpawnLocation;
	protected vector m_vSpawnRotation;
	
	override void OnPostInit(IEntity owner)
	{
		m_pRplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!VerifyRplComponentPresent()) return;
		
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}

	// Inserts method call on parent trigger
	override void EOnInit(IEntity owner)
	{
		SCR_BaseTriggerEntity parentTrigger = SCR_BaseTriggerEntity.Cast(owner);
		if (!parentTrigger)
		{
			Print("SCR_TriggerSpawner cannot hook to trigger, it is not a child of SCR_BaseTriggerEntity!");
			return;
		}
		
		parentTrigger.GetOnActivate().Insert(OnTriggerActivate);
		
		// Grab the parents origin and rotation for the spawn point
		m_vSpawnLocation = owner.GetOrigin();
		m_vSpawnRotation = owner.GetYawPitchRoll();
	}
	
	
	// Removes method call from parent trigger
	override void OnDelete(IEntity owner)
	{
		SCR_BaseTriggerEntity parentTrigger = SCR_BaseTriggerEntity.Cast(owner);
		if (!parentTrigger) return;
		
		parentTrigger.GetOnActivate().Remove(OnTriggerActivate);
	}
	
	
	// The method called on trigger activation
	protected void OnTriggerActivate()
	{
		if (DoSpawn())
		{
			if (!m_sTriggerMessage.IsEmpty())
				SCR_PopUpNotification.GetInstance().PopupMsg(m_sTriggerMessage);
			
			if (m_bOneShot)
				GenericEntity.Cast(GetOwner()).Deactivate();
		}
	}

	
	// Actually spawn the thing
	protected bool DoSpawn()
	{
		if (!VerifyRplComponentPresent())
		{
			Print("SCR_TriggerSpawner cannot spawn group, spawner has no RplComponent!");
			return false;
		}

		if (!m_pRplComponent.IsMaster())
		{
			Print("SCR_TriggerSpawner caught non-master request to spawn!");
			return false;
		}

		Resource prefab = Resource.Load(m_rnSpawnPrefab);
		if (!prefab)
		{
			Print(string.Format("SCR_TriggerSpawner could not load '%1'", prefab));
			return false;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		GetSpawnTransform(spawnParams.Transform);

		IEntity spawnedEntity = GetGame().SpawnEntityPrefab(prefab, GetOwner().GetWorld(), spawnParams);
		if (!spawnedEntity)
		{
			Print(string.Format("SCR_TriggerSpawner could not spawn '%1'", prefab));
			return false;
		}
		
		return true;
	}
	
	protected void GetSpawnTransform(out vector transformMatrix[4])
	{
		vector rotation = m_vSpawnRotation;
		vector yawPitchRoll = Vector(rotation[1], rotation[0], rotation[2]);
		Math3D.AnglesToMatrix(rotation, transformMatrix);		
		transformMatrix[3] = m_vSpawnLocation;
	}
	
	protected bool VerifyRplComponentPresent()
	{
		if (!m_pRplComponent)
		{
			Print("SCR_TriggerSpawner does not have a RplComponent attached!");
			return false;
		}

		return true;
	}
}
