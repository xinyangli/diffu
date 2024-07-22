{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nur-xin = {
      url = "git+https://git.xinyang.life/xin/nur.git";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    pre-commit-hooks = {
      url = "github:cachix/pre-commit-hooks.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, ... }@inputs: with inputs;
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system; overlays = [
          (self: super: {
            mini-gdbstub = nur-xin.legacyPackages.${system}.mini-gdbstub;
          })
        ];
        };
      in
      {
        checks = {
          pre-commit-check = pre-commit-hooks.lib.${system}.run {
            src = ./.;
            hooks = {
              trim-trailing-whitespace.enable = true;
              clang-format = {
                enable = true;
                types_or = pkgs.lib.mkForce [ "c" "c++" ];
              };
              nil.enable = true;
              nixpkgs-fmt.enable = true;
            };
          };
        };
        devShells.default = with pkgs; mkShell {
          inherit (self.checks.${system}.pre-commit-check) shellHook;
          buildInputs = self.checks.${system}.pre-commit-check.enabledPackages;
          packages = [
            clang-tools
            cmake
            gdb
            cli11
            mini-gdbstub
          ];
        };
        packages.default = pkgs.callPackage ./default.nix { };
      }
    );
}
