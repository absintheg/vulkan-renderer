{
  description = "Project Flake";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };
  outputs =
    { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      pkgsShared = with pkgs; [
        vulkan-tools
        clang-tools
        pkg-config
        gnumake
        cmake
        shaderc
      ];
    in
    {
      devShells.${system} = {
        default =
          let
            runtimePkgs = with pkgs; [
              libx11
              libxrandr
              libxinerama
              libxcursor
              libxi
              libGL
              libffi
              wayland
              egl-wayland
              libxkbcommon
              vulkan-loader
            ];
          in
          pkgs.mkShell {
            packages =
              with pkgs;
              [
                wayland-scanner
              ]
              ++ pkgsShared
              ++ runtimePkgs;

            LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath runtimePkgs;
          };

        # Cross compilation shell
        windows =
          let
            pkgsCross = import pkgs.path {
              inherit system;
              crossSystem = {
                config = "x86_64-w64-mingw32";
              };
            };
          in
          pkgs.mkShell {
            packages = [
              pkgs.wineWow64Packages.stable
              pkgsCross.stdenv.cc
            ]
            ++ pkgsShared;

            # Formatter decided it looks good this way.
            shellHook = ''
              			export WINEPREFIX=$(pwd)/.wine
              			'';
          };
      };
    };
}
