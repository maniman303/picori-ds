-- SDL3 cross build using the host's ARM64 multiarch development packages.
package("libsdl3")
    set_homepage("https://www.libsdl.org/")
    set_description("SDL3 for the Picori ROCKNIX cross build")
    set_license("zlib")
    add_urls("https://www.libsdl.org/release/SDL3-$(version).zip",
             "https://github.com/libsdl-org/SDL/releases/download/release-$(version)/SDL3-$(version).zip")
    add_versions("3.4.12", "3d4de8967a49c0451e775a0c1e9022092c19fdef41ba38a83fcf031c5a6496e2")
    add_configs("wayland", {description = "Wayland backend", default = true, type = "boolean"})
    add_configs("x11", {description = "X11 backend", default = false, type = "boolean"})
    add_deps("cmake")
    on_load(function (package)
        package:add("syslinks", "pthread", "dl", "m")
    end)
    on_install("linux", function (package)
        import("package.tools.cmake").install(package, {
            "-DCMAKE_BUILD_TYPE=Release", "-DSDL_SHARED=OFF", "-DSDL_STATIC=ON",
            "-DSDL_TEST_LIBRARY=OFF", "-DSDL_TESTS=OFF", "-DSDL_EXAMPLES=OFF",
            "-DSDL_WAYLAND=ON", "-DSDL_WAYLAND_SHARED=ON", "-DSDL_X11=OFF",
            "-DSDL_KMSDRM=OFF"
        })
    end)
package_end()
