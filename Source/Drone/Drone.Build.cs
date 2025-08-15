using UnrealBuildTool;
   using System.IO;

public class Drone : ModuleRules
   {
    public Drone(ReadOnlyTargetRules Target) : base(Target)
       {
           PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        //    DefaultBuildSettings = BuildSettingsVersion.Latest;
        //    PublicDefinitions.Add("__APPLE__=0");

           
        //    PublicDefinitions.Add("BOOST_USE_WINDOWS_H=0");
        //    PublicDefinitions.Add("NOMINMAX");
        //    PublicDefinitions.Add("__FreeBSD__=0");
        //    PublicDefinitions.Add("__VXWORKS__=0");
        //    PublicDefinitions.Add("__INTERNALDEBUG=0");
        //    PublicDefinitions.Add("_INTERNALDEBUG=0");
           
           // 添加第三方库路径
        string ThirdPartyPath = Path.Combine(ModuleDirectory, "../ThirdParty");
           
           // 添加包含路径
           PublicIncludePaths.AddRange(
               new string[] {
                   Path.Combine(ThirdPartyPath, "PCL/include"),
                   Path.Combine(ThirdPartyPath, "FastDDS/include"),
                Path.Combine(ThirdPartyPath, "Eigen/include"),
                Path.Combine(ThirdPartyPath, "Eigen/include/eigen3"),
                Path.Combine(ThirdPartyPath, "Boost")  // Boost头文件路径
               }
           );
           
           // 添加库路径
        string PCLLibPath = Path.Combine(ThirdPartyPath, "PCL/lib");
        string FastDDSLibPath = Path.Combine(ThirdPartyPath, "FastDDS/lib");
        
        // 使用PublicSystemLibraryPaths
        PublicSystemLibraryPaths.AddRange(
               new string[] {
                PCLLibPath,
                FastDDSLibPath
               }
           );
           
        // 添加库文件 - 使用完整路径
           PublicAdditionalLibraries.AddRange(
               new string[] {
                Path.Combine(PCLLibPath, "pcl_common.lib"),
                Path.Combine(PCLLibPath, "pcl_features.lib"),
                Path.Combine(PCLLibPath, "pcl_filters.lib"),
                Path.Combine(PCLLibPath, "pcl_io.lib"),
                Path.Combine(PCLLibPath, "pcl_io_ply.lib"),
                Path.Combine(PCLLibPath, "pcl_kdtree.lib"),
                Path.Combine(PCLLibPath, "pcl_keypoints.lib"),
                Path.Combine(PCLLibPath, "pcl_ml.lib"),
                Path.Combine(PCLLibPath, "pcl_octree.lib"),
                Path.Combine(PCLLibPath, "pcl_recognition.lib"),
                Path.Combine(PCLLibPath, "pcl_registration.lib"),
                Path.Combine(PCLLibPath, "pcl_sample_consensus.lib"),
                Path.Combine(PCLLibPath, "pcl_search.lib"),
                Path.Combine(PCLLibPath, "pcl_segmentation.lib"),
                Path.Combine(PCLLibPath, "pcl_stereo.lib"),
                Path.Combine(PCLLibPath, "pcl_tracking.lib")
               }
           );
           
           // OpenSSL
        // PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "OpenSSL/lib/libssl.lib"));
        // PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "OpenSSL/lib/libcrypto.lib"));
        // PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "OpenSSL/include"));
        // foonathan_memory
        // PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "foonathan_memory/lib/foonathan_memory-0.7-0.lib"));
        // PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "foonathan_memory/include"));
           
           // 添加UE5依赖模块
            PublicDependencyModuleNames.AddRange(
               new string[] { 
                   "Core", 
                   "CoreUObject", 
                   "Engine", 
                   "InputCore",
                   "Niagara",
                   "NiagaraCore",
                   "RenderCore",
                   "RHI",
                   "PhysicsCore",  // 移除SplineComponent，因为它已经包含在Engine模块中
                   "Json",
                   "JsonUtilities",
                   "Sockets",
                   "Networking"
               }
           );
           
           PrivateDependencyModuleNames.AddRange(
               new string[] {
                   "Slate",
                   "SlateCore",
                   "UMG",
                   "Json",
                   "JsonUtilities",
                   "Sockets",
                   "Networking"
               }
           );
           
           // 添加运行时依赖
        if (Directory.Exists(PCLLibPath))
        {
            RuntimeDependencies.Add("$(BinaryOutputDir)/pcl_*.dll", Path.Combine(PCLLibPath, "pcl_*.dll"));
            RuntimeDependencies.Add("$(BinaryOutputDir)/boost_*.dll", Path.Combine(PCLLibPath, "boost_*.dll"));
        }
        
        // if (Directory.Exists(FastDDSLibPath))
        // {
        //     string fastrtpsDll = Path.Combine(FastDDSLibPath, "fastrtps.dll");
        //     string fastcdrDll = Path.Combine(FastDDSLibPath, "fastcdr.dll");
            
        //     if (File.Exists(fastrtpsDll))
        //     {
        //         RuntimeDependencies.Add("$(BinaryOutputDir)/fastrtps.dll", fastrtpsDll);
        //     }
            
        //     if (File.Exists(fastcdrDll))
        //     {
        //         RuntimeDependencies.Add("$(BinaryOutputDir)/fastcdr.dll", fastcdrDll);
        //     }
        // }
       }
   }