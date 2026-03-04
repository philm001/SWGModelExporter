# SWGModelExporter – Memory Bank

Purpose: Persist key architecture, decisions, fixes, and open items to guide rapid iteration.

Project snapshot:
- Workspace: c:/Users/phill/source/SWGModelExporter
- Timestamp: 2025-08-08T03:18:39Z

Core areas and files:
- FBX skeleton/animation export: [SWGSkeletonExport.cpp](SWGSkeletonExport.cpp), [SWGAnimationParsing.cpp](SWGAnimationParsing.cpp), [SWGMainObject.cpp](SWGMainObject.cpp)
- Static mesh export path (materials/UVs/normals): [mesh_file.cpp](mesh_file.cpp)
- IFF parsing and buffers: [IFF_file.cpp](IFF_file.cpp), [IFF_file.h](IFF_file.h), [base_buffer.cpp](base_buffer.cpp), [base_buffer.h](base_buffer.h)
- TRE resource access: [tre_library.h](tre_library.h), [tre_reader.h](tre_reader.h)
- Logging utility: [Logger.cpp](Logger.cpp), [Logger.h](Logger.h)
- Utility defines: [defines.h](defines.h)

Decisions and conventions:
- FBX axis: Maya Z-Up; scene units in meters.
- Skeleton node transforms use Lcl TRS only; no FBX pre/post rotations to avoid conflicts.
- Quaternion pipeline: normalize inputs; combine as post * bind * pre; decompose via SphericalXYZ; clamp Euler to [-180, 180]; apply Unroll filter as needed.
- Animation curves authored in order: Translation, Rotation, Scale; skip -1000 sentinel values; last frame can use constant interpolation.
- Skin cluster link mode: eNormalize (current); revisit consistency across paths.

Changes already applied (build/runtime impactful):
- Removed duplicate explicit template instantiation tail in [SWGSkeletonExport.cpp](SWGSkeletonExport.cpp).
- Avoided binding a non-const reference to EvaluateLocalTransform temporary in [SWGAnimationParsing.cpp](SWGAnimationParsing.cpp).
- Buffer safety: use resize (not reserve) in _reallocate; fix write_wstring pointer to m_data.data(); pass explicit size to _reallocate in writers; corrected write_double signature to double in [base_buffer.cpp](base_buffer.cpp) and [base_buffer.h](base_buffer.h).

Open issues / follow-ups:
- Unify skinning type and weight normalization across exporters (eRigid+eNormalize vs eTotalOne). Source of truth should be one path.
- Reconcile quaternion combination strategy with any legacy path in objects/animated_object.* to prevent mismatches.
- Consider implementing a dedicated applyAnimationDataToFBX path to consume precomputed curves sequentially; ensure thread-safety vs FBX SDK.
- mesh_file: prefer name.find("NAME") != std::string::npos for clarity/robustness.
- Add includePath for FBX SDK headers to silence IntelliSense squiggles (tooling-only).

Data orientation:
- Bone lookup uses m_bones[0] for LOD0; cluster weights mapped by lowercase joint names.
- Animation::Bone_info flags indicate per-axis translation channels and rotation presence; compressed quats use KFAT/QCHN with format triplet; 100/-1000 sentinels mean skip.

Next reading targets:
- parsers/*, remaining headers, zlib/Boost usages, and any objects/* alternate export paths for parity.

Logging:
- Use LOG_* macros; keep high-signal logs limited to LOD0 and target bones during investigations.

Working TODOs (short):
- Decide and enforce skinning/link mode globally.
- Finalize quaternion/Euler policy across all export paths.
- Extract curve authoring into a single function to avoid drift.
- Sweep for error-prone string::find patterns and replace.

End.