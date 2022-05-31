[ComponentEditorProps(category: "GameScripted/GameMode", description: "Forced time and weather controller.")]
class AGM_ForceTimeAndWeatherComponentClass : SCR_BaseGameModeComponentClass
{
}

//------------------------------------------------------------------------------------------------
/*!
	Forced time and weather component that allows overriding the TimeAndWeatherManager.
	This component must be attached to a SCR_BaseGameMode entity!
*/
class AGM_ForceTimeAndWeatherComponent : SCR_BaseGameModeComponent
{
	//! Manager singleton instance, assigned on first get call
	private static AGM_ForceTimeAndWeatherComponent s_pInstance;

	//! If enabled custom weather Id will be used on session start. Authority only.
	[Attribute(defvalue: "0", desc: "If enabled, custom weather Id will be used. Authority only.", category: "CaptureAndHold: Environment")]
	protected bool m_bUseCustomWeather;

	//! Weather IDs are the same as used in the TimeAndWeatherManager. Weather set on game start. Authority only.
	[Attribute(defvalue: "", desc: "Weather IDs are the same as used in the TimeAndWeatherManager. Weather set on game start. Authority only.", category: "CaptureAndHold: Environment")]
	protected string m_sCustomWeatherId;

	//! If enabled custom time of the day will be used on session start. Authority only.
	[Attribute(defvalue: "0", desc: "If enabled, custom time of the day will be used. Authority only.", category: "CaptureAndHold: Environment")]
	protected bool m_bUseCustomTime;

	//! Time of the day set on game start. Authority only.
	[Attribute(defvalue: "12", desc: "Time of the day set on game start. Authority only.", category: "CaptureAndHold: Environment", params: "0 24 0.01")]
	protected float m_fCustomTimeOfTheDay;

	//------------------------------------------------------------------------------------------------
	/*!
		Initializes this component and hooks up events.
	*/
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Initialize the manager.
	*/
	protected override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (m_bUseCustomTime)
			SetTimeOfTheDay(m_fCustomTimeOfTheDay);

		if (m_bUseCustomWeather)
			SetWeather(m_sCustomWeatherId);
	}

	//------------------------------------------------------------------------------------------------
	/*
		Finds SCR_ForceTimeAndWeatherComponent on current game mode and returns it, or null if none.
	*/
	static AGM_ForceTimeAndWeatherComponent GetActiveComponent()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;

		if (!s_pInstance)
			s_pInstance = AGM_ForceTimeAndWeatherComponent.Cast(gameMode.FindComponent(AGM_ForceTimeAndWeatherComponent));

		return s_pInstance;
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Forcefully sets weather to provided weatherId. Authority only.
	*/
	void SetWeather(string weatherId)
	{
		if (!m_pGameMode.IsMaster())
			return;

		if (weatherId.IsEmpty())
			return;

		TimeAndWeatherManagerEntity weatherManager = GetGame().GetTimeAndWeatherManager();
		if (!weatherManager)
		{
			Print("Cannot initialize weather: TimeAndWeatherManagerEntity not found!", LogLevel.WARNING);
			return;
		}

		weatherManager.ForceWeatherTo(true, weatherId, 0.0);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Forcefully sets time of the day to provided value. Authority only.
	*/
	void SetTimeOfTheDay(float timeOfTheDay)
	{
		if (!m_pGameMode.IsMaster())
			return;

		TimeAndWeatherManagerEntity weatherManager = GetGame().GetTimeAndWeatherManager();
		if (!weatherManager)
		{
			Print("Cannot initialize TimeOfTheDay: TimeAndWeatherManagerEntity not found!", LogLevel.WARNING);
			return;
		}

		weatherManager.SetTimeOfTheDay(timeOfTheDay, true);
	}
}