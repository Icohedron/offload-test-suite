# FindD3D12Linux module
# Finds D3D12 libraries on Linux (either WSL or vkd3d)

if (EXISTS "/usr/lib/wsl/lib/")
    # List of D3D libraries from WSL
    find_library(LIBD3D12 d3d12 HINTS /usr/lib/wsl/lib)
    find_library(LIBDXCORE dxcore HINTS /usr/lib/wsl/lib)
    
    # List of D3D libraries from DirectX-Headers
    find_library(LIBD3DX12-FORMAT-PROPERTIES d3dx12-format-properties)
    find_library(LIBDIRECTX-GUIDS DirectX-Guids)

    if (LIBD3D12 AND LIBDXCORE AND LIBD3DX12-FORMAT-PROPERTIES AND LIBDIRECTX-GUIDS)
        set(D3D12Linux_LIBRARIES
             ${LIBD3D12}
             ${LIBDXCORE}
             ${LIBD3DX12-FORMAT-PROPERTIES}
             ${LIBDIRECTX-GUIDS}
        )
        set(D3D12Linux_IS_WSL TRUE)
    endif()
endif()

if (NOT D3D12Linux_IS_WSL)
    # Fallback to VKD3D
    find_path(VKD3D_INCLUDE_DIR directx/d3d12.h)
    find_path(VKD3D_WRL_INCLUDE_DIR wrl/client.h PATH_SUFFIXES wsl/stubs)
    find_library(LIBVKD3D vkd3d-proton-d3d12)
    find_library(LIBVKD3DCORE vkd3d-proton-d3d12core)

    if (VKD3D_INCLUDE_DIR AND VKD3D_WRL_INCLUDE_DIR AND LIBVKD3D AND LIBVKD3DCORE)
        set(D3D12Linux_LIBRARIES ${LIBVKD3D} ${LIBVKD3DCORE})
        set(D3D12Linux_INCLUDE_DIRS ${VKD3D_INCLUDE_DIR} ${VKD3D_WRL_INCLUDE_DIR})
        set(D3D12Linux_IS_VKD3D TRUE)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(D3D12Linux DEFAULT_MSG D3D12Linux_LIBRARIES)
mark_as_advanced(D3D12Linux_LIBRARIES D3D12Linux_INCLUDE_DIRS)
