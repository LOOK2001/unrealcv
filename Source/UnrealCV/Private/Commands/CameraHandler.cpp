// #include "RealisticRendering.h"
#include "UnrealCVPrivate.h"
#include "CameraHandler.h"
#include "ViewMode.h"
#include "ImageUtils.h"
#include "ImageWrapper.h"
#include "GTCapturer.h"

/**
  * Where to put cameras
  * For each camera in the scene, attach SceneCaptureComponent2D to it
  * So that ground truth can be generated
  */
void FCameraCommandHandler::InitCameraArray()
{
	// TODO: Only support one camera at the beginning
	static TArray<FString>* SupportedModes = nullptr;
	if (SupportedModes == nullptr) // TODO: Get supported modes array from GTCapturer
	{
		SupportedModes = new TArray<FString>();
		SupportedModes->Add(TEXT("depth"));
		SupportedModes->Add(TEXT("")); // This is lit
		// SupportedModes->Add(TEXT("normal"));
	}

	for (auto Mode : *SupportedModes)
	{
		UGTCapturer* Capturer = UGTCapturer::Create(this->Character, Mode);
		this->GTCapturers.Add(Mode, Capturer);
	}
}

FString FCameraCommandHandler::GenerateFilename()
{
	static uint32 NumCaptured = 0;
	NumCaptured++;
	FString Filename = FString::Printf(TEXT("%08d.png"), NumCaptured);
	return Filename;
}

void FCameraCommandHandler::RegisterCommands()
{
	InitCameraArray();

	FDispatcherDelegate Cmd;
	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraViewMode);
	CommandDispatcher->BindCommand("vget /camera/[uint]/[str]", Cmd, "Get snapshot from camera, the third parameter is optional"); // Take a screenshot and return filename

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraViewMode);
	CommandDispatcher->BindCommand("vget /camera/[uint]/[str] [str]", Cmd, "Get snapshot from camera, the third parameter is optional"); // Take a screenshot and return filename

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraLocation);
	CommandDispatcher->BindCommand("vget /camera/[uint]/location", Cmd, "Get camera location");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraRotation);
	CommandDispatcher->BindCommand("vget /camera/[uint]/rotation", Cmd, "Get camera rotation");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetCameraLocation);
	CommandDispatcher->BindCommand("vset /camera/[uint]/location [float] [float] [float]", Cmd, "Set camera location");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetCameraRotation);
	CommandDispatcher->BindCommand("vset /camera/[uint]/rotation [float] [float] [float]", Cmd, "Set camera rotation");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraView);
	CommandDispatcher->BindCommand("vget /camera/[uint]/view", Cmd, "Get snapshot from camera, the second parameter is optional"); // Take a screenshot and return filename

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraView);
	CommandDispatcher->BindCommand("vget /camera/[uint]/view [str]", Cmd, "Get snapshot from camera, the second parameter is optional"); // Take a screenshot and return filename

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraHDR);
	CommandDispatcher->BindCommand("vget /camera/[uint]/hdr", Cmd, "Get hdr capture from camera, the second parameter is optional"); // Take a screenshot and return filename

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraDepth);
	CommandDispatcher->BindCommand("vget /camera/[uint]/depth [str]", Cmd, "Get depth from camera, the second parameter is filename");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraDepth);
	CommandDispatcher->BindCommand("vget /camera/[uint]/depth", Cmd, "Get depth from camera");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraLit);
	CommandDispatcher->BindCommand("vget /camera/[uint]/lit [str]", Cmd, "Get depth from camera");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraLit);
	CommandDispatcher->BindCommand("vget /camera/[uint]/lit", Cmd, "Get depth from camera");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraProjMatrix);
	CommandDispatcher->BindCommand("vget /camera/[uint]/proj_matrix", Cmd, "Get projection matrix");

	Cmd = FDispatcherDelegate::CreateRaw(&FCameraViewMode::Get(), &FCameraViewMode::SetMode);
	CommandDispatcher->BindCommand("vset /mode [str]", Cmd, "Set mode"); // Better to check the correctness at compile time

	Cmd = FDispatcherDelegate::CreateRaw(&FCameraViewMode::Get(), &FCameraViewMode::GetMode);
	CommandDispatcher->BindCommand("vget /mode", Cmd, "Get mode");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetBuffer);
	CommandDispatcher->BindCommand("vget /camera/[uint]/buffer", Cmd, "Get buffer of this camera");
}

FExecStatus FCameraCommandHandler::GetCameraProjMatrix(const TArray<FString>& Args)
{
	// FMatrix& ProjMatrix = FSceneView::ViewProjectionMatrix;
	// this->Character->GetWorld()->GetGameViewport()->Viewport->
	// this->Character
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::SetCameraLocation(const TArray<FString>& Args)
{
	/** The API for Character, Pawn and Actor are different */
	if (Args.Num() == 4) // ID, X, Y, Z
	{
		int32 CameraId = FCString::Atoi(*Args[0]); // TODO: Add support for multiple cameras
		float X = FCString::Atof(*Args[1]), Y = FCString::Atof(*Args[2]), Z = FCString::Atof(*Args[3]);
		FVector Location = FVector(X, Y, Z);

		bool Sweep = false;
		// if sweep is true, the object can not move through another object
		bool Success = Character->SetActorLocation(Location, Sweep, NULL, ETeleportType::TeleportPhysics);

		return FExecStatus::OK();
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::SetCameraRotation(const TArray<FString>& Args)
{
	if (Args.Num() == 4) // ID, Pitch, Roll, Yaw
	{
		int32 CameraId = FCString::Atoi(*Args[0]); // TODO: Add support for multiple cameras
		float Pitch = FCString::Atof(*Args[1]), Yaw = FCString::Atof(*Args[2]), Roll = FCString::Atof(*Args[3]);
		FRotator Rotator = FRotator(Pitch, Yaw, Roll);
		AController* Controller = Character->GetController();
		Controller->ClientSetRotation(Rotator); // Teleport action
		// SetActorRotation(Rotator);  // This is not working

		return FExecStatus::OK();
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::GetCameraRotation(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		int32 CameraId = FCString::Atoi(*Args[0]); // TODO: Add support for multiple cameras
		// FRotator CameraRotation = this->Character->GetActorRotation();  // We need the rotation of the controller
		FRotator CameraRotation = Character->GetControlRotation();
		FString Message = FString::Printf(TEXT("%.3f %.3f %.3f"), CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll);

		return FExecStatus::OK(Message);
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::GetCameraLocation(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		int32 CameraId = FCString::Atoi(*Args[0]); // TODO: Add support for multiple cameras
		FVector CameraLocation = Character->GetActorLocation();
		FString Message = FString::Printf(TEXT("%.3f %.3f %.3f"), CameraLocation.X, CameraLocation.Y, CameraLocation.Z);

		return FExecStatus::OK(Message);
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

// Sync operation for screen capture
bool DoCaptureScreen(UGameViewportClient *ViewportClient, const FString& CaptureFilename)
{
		bool bScreenshotSuccessful = false;
		FViewport* InViewport = ViewportClient->Viewport;
		ViewportClient->GetEngineShowFlags()->SetMotionBlur(false);
		FIntVector Size(InViewport->GetSizeXY().X, InViewport->GetSizeXY().Y, 0);

		bool IsHDR = true;

		if (!IsHDR)
		{
			TArray<FColor> Bitmap;
			bScreenshotSuccessful = GetViewportScreenShot(InViewport, Bitmap);
			// InViewport->ReadFloat16Pixels

			if (bScreenshotSuccessful)
			{
				// Ensure that all pixels' alpha is set to 255
				for (auto& Color : Bitmap)
				{
					Color.A = 255;
				}
				// TODO: Need to blend alpha, a bit weird from screen.

				TArray<uint8> CompressedBitmap;
				FImageUtils::CompressImageArray(Size.X, Size.Y, Bitmap, CompressedBitmap);
				FFileHelper::SaveArrayToFile(CompressedBitmap, *CaptureFilename);
			}
		}
		else // Capture HDR, unable to read float16 data from here. Need to use a rendertarget.
		{
			CaptureFilename.Replace(TEXT(".png"), TEXT(".exr"));
			TArray<FFloat16Color> FloatBitmap;
			FloatBitmap.AddZeroed(Size.X * Size.Y);
			InViewport->ReadFloat16Pixels(FloatBitmap);

			IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
			IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::EXR);

			ImageWrapper->SetRaw(FloatBitmap.GetData(), FloatBitmap.GetAllocatedSize(), Size.X, Size.Y, ERGBFormat::RGBA, 16);
			const TArray<uint8>& PngData = ImageWrapper->GetCompressed(ImageCompression::Uncompressed);
			FFileHelper::SaveArrayToFile(PngData, *CaptureFilename);
		}

		return bScreenshotSuccessful;
}

FExecStatus FCameraCommandHandler::GetCameraViewSync(const FString& FullFilename)
{
	// This can only work within editor
	// Reimplement a GameViewportClient is required according to the discussion from here
	// https://forums.unrealengine.com/showthread.php?50857-FViewPort-ReadPixels-crash-while-play-on-quot-standalone-Game-quot-mode
	UGameViewportClient* ViewportClient = Character->GetWorld()->GetGameViewport();
	if (DoCaptureScreen(ViewportClient, FullFilename))
	{
		return FExecStatus::OK(FullFilename);
	}
	else
	{
		return FExecStatus::Error("Fail to capture screen");
	}
}

FExecStatus FCameraCommandHandler::GetCameraViewAsyncQuery(const FString& FullFilename)
{
		// Method 1: Use custom ViewportClient
		// UMyGameViewportClient* ViewportClient = (UMyGameViewportClient*)Character->GetWorld()->GetGameViewport();
		// ViewportClient->CaptureScreen(FullFilename);

		// Method2: System screenshot function
		FScreenshotRequest::RequestScreenshot(FullFilename, false, false); // This is an async operation

		// Implement 2, Start async and query
		// FPromiseDelegate PromiseDelegate = FPromiseDelegate::CreateRaw(this, &FCameraCommandHandler::CheckStatusScreenshot);
		FPromiseDelegate PromiseDelegate = FPromiseDelegate::CreateLambda([FullFilename]()
		{
			if (FScreenshotRequest::IsScreenshotRequested())
			{
				return FExecStatus::Pending();
			}
			else
			{
				FString DiskFilename = IFileManager::Get().GetFilenameOnDisk(*FullFilename); // This is important
				// See: https://wiki.unrealengine.com/Packaged_Game_Paths,_Obtain_Directories_Based_on_Executable_Location.
				return FExecStatus::OK(DiskFilename);
			}
		});

		// Method3: USceneCaptureComponent2D, inspired by StereoPanorama plugin

		FExecStatus ExecStatusQuery = FExecStatus::AsyncQuery(FPromise(PromiseDelegate));
		/* This is only valid within custom viewport client
		UGameViewportClient* ViewportClient = Character->GetWorld()->GetGameViewport();
		ViewportClient->OnScreenshotCaptured().Clear(); // This is required to handle the filename issue.
		ViewportClient->OnScreenshotCaptured().AddLambda(
			[FullFilename](int32 SizeX, int32 SizeY, const TArray<FColor>& Bitmap)
		{
			// Save bitmap to disk
			TArray<FColor>& RefBitmap = const_cast<TArray<FColor>&>(Bitmap);
			for (auto& Color : RefBitmap)
			{
				Color.A = 255;
			}

			TArray<uint8> CompressedBitmap;
			FImageUtils::CompressImageArray(SizeX, SizeY, RefBitmap, CompressedBitmap);
			FFileHelper::SaveArrayToFile(CompressedBitmap, *FullFilename);
		});
		*/
		return ExecStatusQuery;

}

/*
FExecStatus FCameraCommandHandler::GetCameraViewAsyncCallback(const FString& FullFilename)
{
		FScreenshotRequest::RequestScreenshot(FullFilename, false, false); // This is an async operation

		// async implement 1, Start async and callback
		FExecStatus ExecStatusAsyncCallback = FExecStatus::AsyncCallback();
		int32 TaskId = ExecStatusAsyncCallback.TaskId;
		UMyGameViewportClient* ViewportClient = (UMyGameViewportClient*)Character->GetWorld()->GetGameViewport();
		ViewportClient->OnScreenshotCaptured().Clear();
		ViewportClient->OnScreenshotCaptured().AddLambda(
			[FullFilename, TaskId](int32 SizeX, int32 SizeY, const TArray<FColor>& Bitmap)
		{
			// Save bitmap to disk
			TArray<FColor>& RefBitmap = const_cast<TArray<FColor>&>(Bitmap);
			TArray<uint8> CompressedBitmap;
			FImageUtils::CompressImageArray(SizeX, SizeY, RefBitmap, CompressedBitmap);
			FFileHelper::SaveArrayToFile(CompressedBitmap, *FullFilename);

			FString Message = FullFilename;
			// FAsyncTaskPool::Get().CompleteTask(TaskId, Message); // int32 is easy to pass around in lamdba

			// Mark this async task finished
			// ExecStatusAsyncCallback.MessageBody = FullFilename;
			// ExecStatusAsyncCallback.OnFinished().Execute(); // FullFilename is the message to return
		});
		return ExecStatusAsyncCallback;
}
*/

FExecStatus FCameraCommandHandler::GetCameraViewMode(const TArray<FString>& Args)
{
	if (Args.Num() <= 3) // The first is camera id, the second is ViewMode
	{
		FString CameraId = Args[0];
		FString ViewMode = Args[1];

		// TODO: Disable buffer visualization
		static IConsoleVariable* ICVar1 = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationDumpFrames"));
		// r.BufferVisualizationDumpFramesAsHDR
		ICVar1->Set(0, ECVF_SetByCode);
		static IConsoleVariable* ICVar2 = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationDumpFramesAsHDR"));
		ICVar2->Set(0, ECVF_SetByCode);

		/*
		TArray<FString> Args1;
		Args1.Add(ViewMode);
		FViewMode::Get().SetMode(Args1);
		*/
		// Use command dispatcher is more universal
		FExecStatus ExecStatus = CommandDispatcher->Exec(FString::Printf(TEXT("vset /mode %s"), *ViewMode));
		if (ExecStatus != FExecStatusType::OK)
		{
			return ExecStatus;
		}

		TArray<FString> Args2;
		Args2.Add(CameraId);
		if (Args.Num() == 3)
		{
			FString Fullfilename = Args[2];
			Args2.Add(Fullfilename);
		}
		ExecStatus = GetCameraView(Args2);
		return ExecStatus;
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::GetCameraView(const TArray<FString>& Args)
{
	if (Args.Num() <= 2)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);

		FString FullFilename, Filename;
		if (Args.Num() == 1)
		{
			Filename = GenerateFilename();
		}
		if (Args.Num() == 2)
		{
			Filename = Args[1];
		}
		const FString Dir = FPlatformProcess::BaseDir(); // TODO: Change this to screen capture folder
		// const FString Dir = FPaths::ScreenShotDir();
		FullFilename = FPaths::Combine(*Dir, *Filename);

		return this->GetCameraViewAsyncQuery(FullFilename);
		// return this->GetCameraViewSync(FullFilename);
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::GetBuffer(const TArray<FString>& Args)
{
	// Initialize console variables.
	static IConsoleVariable* ICVar1 = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationDumpFrames"));
	// r.BufferVisualizationDumpFramesAsHDR
	ICVar1->Set(1, ECVF_SetByCode);
	static IConsoleVariable* ICVar2 = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationDumpFramesAsHDR"));
	ICVar2->Set(1, ECVF_SetByCode);


	/*
	FHighResScreenshotConfig Config = GetHighResScreenshotConfig();
	Config.bCaptureHDR = true;
	Config.bDumpBufferVisualizationTargets = true;
	*/

	// UGameViewportClient* GameViewportClient = this->GetWorld()->GetGameViewport();
	// GameViewportClient->EngineShowFlags.VisualizeBuffer = true;

	GIsHighResScreenshot = true;

	/*
	FString ViewMode = "SceneDepth";
	static IConsoleVariable* ICVar = IConsoleManager::Get().FindConsoleVariable(FBufferVisualizationData::GetVisualizationTargetConsoleCommandName());
	if (ICVar)
	{
		ICVar->Set(*ViewMode, ECVF_SetByCode); // TODO: Should wait here for a moment.
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("The BufferVisualization is not correctly configured."));
	}
	*/

	// Get the viewport
	// FSceneViewport* SceneViewport = this->GetWorld()->GetGameViewport()->GetGameViewport();
	// SceneViewport->TakeHighResScreenShot();

	// OnFire();
	// return FExecStatus::InvalidArgument;
	return FExecStatus::OK();
}

FExecStatus FCameraCommandHandler::GetCameraHDR(const TArray<FString>& Args)
{
	return FExecStatus::OK();
}

FString GetDiskFilename(FString Filename)
{
	FString DiskFilename = IFileManager::Get().GetFilenameOnDisk(*Filename); // This is important
	return DiskFilename;
}

FExecStatus FCameraCommandHandler::GetCameraDepth(const TArray<FString>& Args)
{
	if (Args.Num() <= 2)
	{
		FString CameraId = Args[0];
		FString Filename;
		if (Args.Num() == 1)
		{
			Filename = GenerateFilename();
		}
		if (Args.Num() == 2)
		{
			Filename = Args[1];
		}
		UGTCapturer* GTCapturer = GTCapturers.FindRef(TEXT("depth"));
		GTCapturer->Capture(*Filename);

		// TODO: Check IsPending is problematic.
		FPromiseDelegate PromiseDelegate = FPromiseDelegate::CreateLambda([Filename, GTCapturer]()
		{
			if (GTCapturer->IsPending())
			{
				return FExecStatus::Pending();
			}
			else
			{
				
				return FExecStatus::OK(GetDiskFilename(Filename));
			}

		});
		return FExecStatus::AsyncQuery(FPromise(PromiseDelegate));
	}
	return FExecStatus::InvalidArgument;
}


FExecStatus FCameraCommandHandler::GetCameraLit(const TArray<FString>& Args)
{
	if (Args.Num() <= 2)
	{
		FString CameraId = Args[0];
		FString Filename;
		if (Args.Num() == 1)
		{
			Filename = GenerateFilename();
		}
		if (Args.Num() == 2)
		{
			Filename = Args[1];
		}
		UGTCapturer* GTCapturer = GTCapturers.FindRef(TEXT(""));
		GTCapturer->Capture(*Filename);

		// TODO: Check IsPending is problematic.
		FPromiseDelegate PromiseDelegate = FPromiseDelegate::CreateLambda([Filename, GTCapturer]()
		{
			if (GTCapturer->IsPending())
			{
				return FExecStatus::Pending();
			}
			else
			{
				
				return FExecStatus::OK(GetDiskFilename(Filename));
			}

		});
		return FExecStatus::AsyncQuery(FPromise(PromiseDelegate));
	}
	return FExecStatus::InvalidArgument;
}


/*
FExecStatus FCameraCommandHandler::GetCameraHDR(const TArray<FString>& Args)
{
	if (Args.Num() <= 2)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		static uint32 NumCaptured = 0;

		FString FullFilename, Filename;
		if (Args.Num() == 1)
		{
			NumCaptured++;
			Filename = FString::Printf(TEXT("%04d.exr"), NumCaptured);
		}
		if (Args.Num() == 2)
		{
			Filename = Args[1];
		}
		const FString Dir = FPlatformProcess::BaseDir(); // TODO: Change this to screen capture folder
		// const FString Dir = FPaths::ScreenShotDir();
		FullFilename = FPaths::Combine(*Dir, *Filename);

		// USceneCaptureComponent2D* CaptureComponent =  CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
		// See: SceneCapture2D.h:19
		// DEPRECATED_FORGAME(4.6, "CaptureComponent2D should not be accessed directly, please use GetCaptureComponent2D() function instead. CaptureComponent2D will soon be private and your code will not compile.")
		USceneCaptureComponent2D* CaptureComponent = NewObject<USceneCaptureComponent2D>();
		CaptureComponent->TextureTarget = NewObject<UTextureRenderTarget2D>();
		// Check: SceneCaptureRendering.cpp
		// CaptureComponent->TextureTarget

		CaptureComponent->RegisterComponentWithWorld(GWorld);
		CaptureComponent->AddToRoot();

		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);


		int Width = 640, Height = 480;
		TArray<FColor> Image;
		Image.AddZeroed(Width * Height);


		// This is for png
		auto Data = Image.GetData();
		auto Size = Image.GetAllocatedSize();

		// According to ab237f46dc0eee40263acbacbe938312eb0dffbb/LevelTick.cpp:1341, need to wait tick of the scene, then the RenderTarget will be filled.

		FlushRenderingCommands();
		// Read SurfaceData
		// SCOPE_CYCLE_COUNTER, to evaluate performance
		FTextureRenderTargetResource* RenderTarget = CaptureComponent->TextureTarget->GameThread_GetRenderTargetResource();
		RenderTarget->ReadPixels(Image);


		ImageWrapper->SetRaw(Data, Size, Width, Height, ERGBFormat::BGRA, 8);
		const TArray<uint8>& PngData = ImageWrapper->GetCompressed(100);
		FFileHelper::SaveArrayToFile(PngData, *FullFilename);

	}
	return FExecStatus::InvalidArgument;
}
*/
