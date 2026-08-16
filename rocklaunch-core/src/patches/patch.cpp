#include "rocklaunch/core/patches/patch.h"

namespace rocklaunch
{

std::string PatchOperationTypeName(PatchOperationType type)
{
    switch (type) {
    case PatchOperationType::EditFile:
        return "edit";
    case PatchOperationType::CopyFile:
        return "copy";
    case PatchOperationType::RemoveFile:
        return "remove";
    case PatchOperationType::RestoreFile:
        return "restore";
    case PatchOperationType::SetEnv:
        return "env";
    }

    return "unknown";
}

} // namespace rocklaunch
