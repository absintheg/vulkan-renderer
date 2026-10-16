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
        clang-tools
        pkg-config
        gnumake
        cmake
        shaderc

        vulkan-tools
        vulkan-loader
      ];
    in
    {
      devShells.${system} = {
        default = pkgs.mkShell {
          packages =
            with pkgs;
            [
              libx11
              libxrandr
              libxinerama
              libxcursor
              libxi
              libxext
              libGL
              wayland
              wayland-scanner
              egl-wayland
              libxkbcommon
            ]
            ++ pkgsShared;
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
