// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "Interop/Connection/SpatialEventTracer.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "UObject/Class.h"
#include "EngineMinimal.h"
#include "GameFramework/Actor.h"
#include <WorkerSDK/improbable/c_trace.h>

DEFINE_LOG_CATEGORY(LogSpatialEventTracer);

using namespace SpatialGDK;
using namespace worker::c;

void MyTraceCallback(void* UserData, const Trace_Item* Item)
{
	switch (Item->item_type)
	{
	case TRACE_ITEM_TYPE_EVENT:
	{
		const Trace_Event& Event = Item->item.event;

		// TODO: remove temporary filtering?
		if (Event.type == FString("network.receive_raw_message") ||
			Event.type == FString("network.receive_udp_datagram") ||
			Event.type == FString("network.send_raw_message") ||
			Event.type == FString("network.send_udp_datagram") ||
			Event.type == FString("worker.dequeue_op") ||
			Event.type == FString("worker.enqueue_op"))
		{
			return;
		}

#if 0
		FString HexStr;
		for (int i = 0; i < 16; i++)
		{
			char b[32];
			unsigned int x = (unsigned char)Event.span_id.data[i];
			sprintf(b, "%0x", x);
			HexStr += ANSI_TO_TCHAR(b);
		}
		UE_LOG(LogSpatialEventTracer, Log, TEXT("Span: %s, Type: %s, Message: %s, Timestamp: %lu"), *HexStr, *FString(Event.type), *FString(Event.message), Event.unix_timestamp_millis);
#endif

		if (Event.data != nullptr)
		{
			uint32_t DataFieldCount = Trace_EventData_GetFieldCount(Event.data);

			TArray<const char*> Keys;
			Keys.SetNumUninitialized(DataFieldCount);
			TArray<const char*> Values;
			Values.SetNumUninitialized(DataFieldCount);

			Trace_EventData_GetStringFields(Event.data, Keys.GetData(), Values.GetData());
			for (uint32_t i = 0; i < DataFieldCount; ++i)
			{
				//UE_LOG(LogSpatialEventTracer, Warning, TEXT("%s : %s"), ANSI_TO_TCHAR(Keys[i]), ANSI_TO_TCHAR(Values[i]));
			}
		}

		break;
	}
	//case TRACE_ITEM_TYPE_SPAN:
	//{
	//	const Trace_Span& Span = Item->item.span;
	//	//UE_LOG(LogSpatialEventTracer, Warning, TEXT("Span: %s"), *FString(Span.id.data));
	//	unsigned long int span1 = *reinterpret_cast<const unsigned long int*>(&Span.id.data[0]);
	//	unsigned long int span2 = *reinterpret_cast<const unsigned long int*>(&Span.id.data[8]);
	//	UE_LOG(LogSpatialEventTracer, Warning, TEXT("Span: %ul%ul"), span1, span2);
	//	break;
	//}
	default:
	{
		break;
	}
	}
}

SpatialSpanId::SpatialSpanId(Trace_EventTracer* InEventTracer)
	: CurrentSpanId{}
	, EventTracer(InEventTracer)
{
	CurrentSpanId = Trace_EventTracer_AddSpan(EventTracer, nullptr, 0);
	Trace_EventTracer_SetActiveSpanId(EventTracer, CurrentSpanId);
}

SpatialSpanId::~SpatialSpanId()
{
	Trace_EventTracer_UnsetActiveSpanId(EventTracer);
}

SpatialEventTracer::SpatialEventTracer(UWorld* World)
	: NetDriver(Cast<USpatialNetDriver>(World->GetNetDriver()))
{
	Trace_EventTracer_Parameters parameters = {};
	parameters.callback = MyTraceCallback;
	EventTracer = Trace_EventTracer_Create(&parameters);
	Trace_EventTracer_Enable(EventTracer);
}

SpatialEventTracer::~SpatialEventTracer()
{
	Trace_EventTracer_Disable(EventTracer);
	Trace_EventTracer_Destroy(EventTracer);
}

SpatialSpanId SpatialEventTracer::CreateActiveSpan()
{
	return SpatialSpanId(EventTracer);
}

TOptional<Trace_SpanId> SpatialEventTracer::TraceEvent(const SpatialGDKEvent& Event, const worker::c::Trace_SpanId* Cause)
{
	Trace_SpanId CurrentSpanId;
	if (Cause)
	{
		CurrentSpanId = Trace_EventTracer_AddSpan(EventTracer, Cause, 1); // This only works for count=1
	}
	else
	{
		CurrentSpanId = Trace_EventTracer_AddSpan(EventTracer, nullptr, 0);
	}

	Trace_Event TraceEvent{ CurrentSpanId, 0, TCHAR_TO_ANSI(*Event.Message), TCHAR_TO_ANSI(*Event.Type), nullptr };

	if (Trace_EventTracer_ShouldSampleEvent(EventTracer, &TraceEvent))
	{
		Trace_EventData* EventData = Trace_EventData_Create();
		for (const auto& Elem : Event.Data)
		{
			const char* Key = TCHAR_TO_ANSI(*Elem.Key);
			const char* Value = TCHAR_TO_ANSI(*Elem.Value);
			Trace_EventData_AddStringFields(EventData, 1, &Key, &Value);
		}
		TraceEvent.data = EventData;
		Trace_EventTracer_AddEvent(EventTracer, &TraceEvent);
		Trace_EventData_Destroy(EventData);
		return CurrentSpanId;
	}
	return {};
}

void SpatialEventTracer::Enable()
{
	Trace_EventTracer_Enable(EventTracer);
	bEnalbed = true;
}

void SpatialEventTracer::Disable()
{
	Trace_EventTracer_Disable(EventTracer);
	bEnalbed = false;
}

SpatialGDK::SpatialGDKEvent SpatialGDK::ConstructEvent(Worker_RequestId RequestID, bool bSuccess)
{
	SpatialGDKEvent Event;
	Event.Message = "";
	Event.Type = "CommandResponse";
	Event.Data.Add("RequestID", FString::Printf(TEXT("%li"), RequestID));
	Event.Data.Add("Success", bSuccess ? TEXT("true") : TEXT("false"));
	return Event;
}

SpatialGDK::SpatialGDKEvent SpatialGDK::ConstructEvent(const AActor* Actor, const UObject* TargetObject, const UFunction* Function, Worker_CommandResponseOp ResponseOp)
{
	SpatialGDKEvent Event;
	Event.Message = "";
	Event.Type = "ComponentUpdate";
	if (Actor != nullptr)
	{
		Event.Data.Add("Actor", Actor->GetName());
		Event.Data.Add("Position", Actor->GetActorTransform().GetTranslation().ToString());
	}
	if (TargetObject != nullptr)
	{
		Event.Data.Add("TargetObject", TargetObject->GetName());
	}
	Event.Data.Add("Function", Function->GetName());
	//Event.Data.Add("ResponseOp", ResponseOp);
	return Event;
}

SpatialGDK::SpatialGDKEvent SpatialGDK::ConstructEvent(const AActor* Actor, const FString& Message, Worker_CreateEntityResponseOp ResponseOp)
{
	SpatialGDKEvent Event;
	Event.Message = Message;
	Event.Type = "CreateEntityOp";
	if (Actor != nullptr)
	{
		Event.Data.Add("Actor", Actor->GetName());
		Event.Data.Add("Position", Actor->GetActorTransform().GetTranslation().ToString());
	}
	//Event.Data.Add("ResponseOp", FString::Printf(TEXT("%lu"), ResponseOp));
	return Event;
}

SpatialGDK::SpatialGDKEvent SpatialGDK::ConstructEvent(const AActor* Actor, const FString& Type, Worker_CommandResponseOp ResponseOp)
{
	SpatialGDKEvent Event;
	Event.Message = "";
	Event.Type = Type;
	//Event.Data.Add("ResponseOp", ResponseOp);
	return Event;
}

SpatialGDK::SpatialGDKEvent SpatialGDK::ConstructEvent(const AActor* Actor, const UObject* TargetObject, const UFunction* Function, TraceKey TraceId, Worker_RequestId RequestID)
{
	SpatialGDKEvent Event;
	Event.Message = "";
	Event.Type = "ComponentUpdate";
	if (Actor != nullptr)
	{
		Event.Data.Add("Actor", Actor->GetName());
		Event.Data.Add("Position", Actor->GetActorTransform().GetTranslation().ToString());
	}
	if (TargetObject != nullptr)
	{
		Event.Data.Add("TargetObject", TargetObject->GetName());
	}
	Event.Data.Add("Function", Function->GetName());
	Event.Data.Add("TraceId", FString::Printf(TEXT("%i"), TraceId));
	Event.Data.Add("RequestID", FString::Printf(TEXT("%li"), RequestID));
	return Event;
}

SpatialGDK::SpatialGDKEvent SpatialGDK::ConstructEvent(const AActor* Actor, const FString& Type, Worker_RequestId RequestID)
{
	SpatialGDKEvent Event;
	Event.Message = "";
	Event.Type = Type;
	Event.Data.Add("RequestID", FString::Printf(TEXT("%li"), RequestID));
	return Event;
}

SpatialGDK::SpatialGDKEvent SpatialGDK::ConstructEvent(const AActor* Actor, const UObject* TargetObject, Worker_ComponentId ComponentId)
{
	SpatialGDKEvent Event;
	Event.Message = "";
	Event.Type = "ComponentUpdate";
	if (Actor != nullptr)
	{
		Event.Data.Add("Actor", Actor->GetName());
		Event.Data.Add("Position", Actor->GetActorTransform().GetTranslation().ToString());
	}
	if (TargetObject != nullptr)
	{
		Event.Data.Add("TargetObject", TargetObject->GetName());
	}
	Event.Data.Add("ComponentId", FString::Printf(TEXT("%u"), ComponentId));
	return Event;
}

SpatialGDK::SpatialGDKEvent SpatialGDK::ConstructEvent(const AActor* Actor, ENetRole Role)
{
	SpatialGDKEvent Event;
	Event.Message = "";
	Event.Type = "AuthorityChange";
	if (Actor != nullptr)
	{
		Event.Data.Add("Actor", Actor->GetName());
		Event.Data.Add("Position", Actor->GetActorTransform().GetTranslation().ToString());
	}
	switch (Role)
	{
	case ROLE_SimulatedProxy:
		Event.Data.Add("NewRole", "ROLE_None");
		break;
	case ROLE_AutonomousProxy:
		Event.Data.Add("NewRole", "ROLE_AutonomousProxy");
		break;
	case ROLE_Authority:
		Event.Data.Add("NewRole", "ROLE_Authority");
		break;
	default:
		Event.Data.Add("NewRole", "Invalid Role");
		break;
	}

	return Event;
}

SpatialGDK::SpatialGDKEvent SpatialGDK::ConstructEvent(const AActor* Actor, Worker_EntityId EntityId, Worker_RequestId RequestID)
{
	SpatialGDKEvent Event;
	Event.Message = "";
	Event.Type = "RetireEntity";
	if (Actor != nullptr)
	{
		Event.Data.Add("Actor", Actor->GetName());
		Event.Data.Add("Position", Actor->GetActorTransform().GetTranslation().ToString());
	}
	Event.Data.Add("CreateEntityRequestId", FString::Printf(TEXT("%lu"), RequestID));
	return Event;
}

SpatialGDK::SpatialGDKEvent SpatialGDK::ConstructEvent(const AActor* Actor, Worker_RequestId CreateEntityRequestId)
{
	SpatialGDKEvent Event;
	Event.Message = "";
	Event.Type = "CreateEntity";
	if (Actor != nullptr)
	{
		Event.Data.Add("Actor", Actor->GetName());
		Event.Data.Add("Position", Actor->GetActorTransform().GetTranslation().ToString());
	}
	Event.Data.Add("CreateEntityRequestId", FString::Printf(TEXT("%li"), CreateEntityRequestId));
	return Event;
}
