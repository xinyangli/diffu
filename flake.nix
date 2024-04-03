{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    pre-commit-hooks = {
      url = "github:cachix/pre-commit-hooks.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, ... }@inputs: with inputs;
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
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
          ];
        };
      }
    );
}

