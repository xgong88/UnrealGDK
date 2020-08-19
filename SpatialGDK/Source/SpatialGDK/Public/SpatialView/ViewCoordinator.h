// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

//#include "SpatialView/CommandRetryHandlerImpl.h"
#include "SpatialView/CommandRetryHandler.h"
#include "SpatialView/ConnectionHandler/AbstractConnectionHandler.h"
#include "SpatialView/WorkerView.h"
#include "Templates/UniquePtr.h"

namespace SpatialGDK
{
class ViewCoordinator
{
public:
	explicit ViewCoordinator(TUniquePtr<AbstractConnectionHandler> ConnectionHandler);

	~ViewCoordinator();

	// Moveable, not copyable.
	ViewCoordinator(const ViewCoordinator&) = delete;
	ViewCoordinator(ViewCoordinator&&) = delete;
	ViewCoordinator& operator=(const ViewCoordinator&) = delete;
	ViewCoordinator& operator=(ViewCoordinator&&) = delete;

	void Advance(float DeltaTimeS);
	const ViewDelta& GetViewDelta();
	const EntityView& GetView();
	void FlushMessagesToSend();

	const FString& GetWorkerId() const;
	const TArray<FString>& GetWorkerAttributes() const;

	void SendAddComponent(Worker_EntityId EntityId, ComponentData Data);
	void SendComponentUpdate(Worker_EntityId EntityId, ComponentUpdate Update);
	void SendRemoveComponent(Worker_EntityId EntityId, Worker_ComponentId ComponentId);
	Worker_RequestId SendReserveEntityIdsRequest(uint32 NumberOfEntityIds, TOptional<uint32> TimeoutMillis = {});
	Worker_RequestId SendCreateEntityRequest(TArray<ComponentData> EntityComponents, TOptional<Worker_EntityId> EntityId,
											 TOptional<uint32> TimeoutMillis = {});
	Worker_RequestId SendDeleteEntityRequest(Worker_EntityId EntityId, TOptional<uint32> TimeoutMillis = {});
	Worker_RequestId SendEntityQueryRequest(EntityQuery Query, TOptional<uint32> TimeoutMillis = {});
	Worker_RequestId SendEntityCommandRequest(Worker_EntityId EntityId, CommandRequest Request, TOptional<uint32> TimeoutMillis = {});
	void SendEntityCommandResponse(Worker_RequestId RequestId, CommandResponse Response);
	void SendEntityCommandFailure(Worker_RequestId RequestId, FString Message);
	void SendMetrics(SpatialMetrics Metrics);
	void SendLogMessage(Worker_LogLevel Level, const FName& LoggerName, FString Message);

	Worker_RequestId SendReserveEntityIdsRequest(uint32 NumberOfEntityIds, FRetryData RetryData);
	Worker_RequestId SendCreateEntityRequest(TArray<ComponentData> EntityComponents, TOptional<Worker_EntityId> EntityId,
											 FRetryData RetryData);
	Worker_RequestId SendDeleteEntityRequest(Worker_EntityId EntityId, FRetryData RetryData);
	Worker_RequestId SendEntityQueryRequest(EntityQuery Query, FRetryData RetryData);
	Worker_RequestId SendEntityCommandRequest(Worker_EntityId EntityId, CommandRequest Request, FRetryData RetryData);

private:
	WorkerView View;
	TUniquePtr<AbstractConnectionHandler> ConnectionHandler;
	Worker_RequestId NextRequestId;

	FReserveEntityIdsRetryHandler ReserveEntityIdRetryHandler;
	FCreateEntityRetryHandler CreateEntityRetryHandler;
	FDeleteEntityRetryHandler DeleteEntityRetryHandler;
	FEntityQueryRetryHandler EntityQueryRetryHandler;
	FEntityCommandRetryHandler EntityCommandRetryHandler;
};

} // namespace SpatialGDK
