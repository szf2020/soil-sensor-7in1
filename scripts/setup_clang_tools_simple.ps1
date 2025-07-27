#!/usr/bin/env pwsh
# =============================================================================
# CLANG TOOLS SETUP SCRIPT - JXCT Soil Sensor
# =============================================================================
# Author: JXCT Development Team
# Version: 1.0.0
# Date: 27.07.2025
# Description: Simple setup script for Clang tools

param(
    [switch]$InstallTools,
    [switch]$UpdateConfigs,
    [switch]$SetupLSP,
    [switch]$All
)

function Test-Command {
    param([string]$Command)
    try {
        Get-Command $Command -ErrorAction Stop | Out-Null
        return $true
    } catch {
        return $false
    }
}

function Install-ClangTools {
    Write-Host "Installing Clang tools..." -ForegroundColor Cyan
    
    $tools = @{
        "clangd" = "LLVM.LLVM"
        "clang-tidy" = "LLVM.LLVM"
        "clang-format" = "LLVM.LLVM"
    }
    
    foreach ($tool in $tools.Keys) {
        Write-Host "Checking $tool..." -ForegroundColor Cyan
        if (Test-Command $tool) {
            Write-Host "OK: $tool is already installed" -ForegroundColor Green
        } else {
            Write-Host "Installing $tool..." -ForegroundColor Yellow
            try {
                winget install $tools[$tool] --accept-source-agreements --accept-package-agreements
                if (Test-Command $tool) {
                    Write-Host "OK: $tool installed successfully" -ForegroundColor Green
                } else {
                    Write-Host "ERROR: Failed to install $tool" -ForegroundColor Red
                }
            } catch {
                Write-Host "ERROR: Installation failed for $tool" -ForegroundColor Red
            }
        }
    }
}

function Update-ClangConfigs {
    Write-Host "Updating Clang configurations..." -ForegroundColor Cyan
    
    # Update .clang-format
    $clangFormatConfig = @"
BasedOnStyle: Google
Language: Cpp
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 120
AllowShortFunctionsOnASingleLine: Empty
DerivePointerAlignment: false
PointerAlignment: Left
SpacesInAngles: false
SpaceBeforeCpp11BracedList: false
BreakBeforeBraces: Allman
SortIncludes: true
IncludeBlocks: Preserve
"@
    
    $clangFormatConfig | Out-File -FilePath ".clang-format" -Encoding UTF8
    Write-Host "OK: .clang-format updated" -ForegroundColor Green
    
    # Create .clang-tidy
    $clangTidyConfig = @"
Checks: >
  -*,
  bugprone-*,
  misc-*,
  performance-*,
  clang-analyzer-*,
  modernize-use-trailing-return-type,
  readability-identifier-naming,
  -bugprone-easily-swappable-parameters,
  -readability-convert-member-functions-to-static,
  -misc-use-internal-linkage

WarningsAsErrors: ''
HeaderFilterRegex: ''
AnalyzeTemporaryDtors: false

CheckOptions:
  - key: misc-non-private-member-variables-in-classes.IgnoreClassesWithAllMemberVariablesBeingPublic
    value: 'true'
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: lower_case
  - key: readability-identifier-naming.VariableCase
    value: lower_case
  - key: readability-identifier-naming.ConstantCase
    value: UPPER_CASE
  - key: readability-identifier-naming.ParameterCase
    value: lower_case
  - key: readability-identifier-naming.MacroDefinitionCase
    value: UPPER_CASE

SystemHeaders: false
HeaderFilterRegex: '^(include|src)/.*\.(h|hpp|cpp)$'
"@
    
    $clangTidyConfig | Out-File -FilePath ".clang-tidy" -Encoding UTF8
    Write-Host "OK: .clang-tidy created" -ForegroundColor Green
}

function Setup-LSP {
    Write-Host "Setting up LSP for Cursor/VS Code..." -ForegroundColor Cyan
    
    # Create .vscode directory if it doesn't exist
    if (!(Test-Path ".vscode")) {
        New-Item -ItemType Directory -Path ".vscode" | Out-Null
    }
    
    # Create settings.json with clangd configuration
    $vscodeSettings = @{
        "files.associations" = @{
            "*.h" = "cpp"
            "*.cpp" = "cpp"
            "*.ino" = "cpp"
            "*.hpp" = "cpp"
        }
        "C_Cpp.default.cppStandard" = "c++17"
        "C_Cpp.default.includePath" = @(
            "`${workspaceFolder}/include",
            "`${workspaceFolder}/src",
            "`${workspaceFolder}/test/stubs"
        )
        "C_Cpp.default.defines" = @(
            "ESP32",
            "DEBUG_MODE",
            "INFO_MODE"
        )
        "C_Cpp.intelliSenseEngine" = "default"
        "C_Cpp.errorSquiggles" = "enabled"
        "platformio-ide.useBuiltinPIOCore" = $true
        "platformio-ide.useDevelopmentPIOCore" = $false
        "clangd.arguments" = @(
            "--background-index",
            "--clang-tidy",
            "--completion-style=detailed",
            "--header-insertion=iwyu",
            "--suggest-missing-includes",
            "--compile-commands-dir=build"
        )
        "clangd.checkUpdates" = $true
        "clangd.onConfigChanged" = "restart"
    }
    
    $vscodeSettings | ConvertTo-Json -Depth 10 | Out-File -FilePath ".vscode/settings.json" -Encoding UTF8
    Write-Host "OK: .vscode/settings.json updated with clangd settings" -ForegroundColor Green
}

function Update-PlatformIOConfig {
    Write-Host "Updating platformio.ini..." -ForegroundColor Cyan
    
    # Add new section for enhanced analysis
    $platformioConfig = @"

; =============================================================================
; ENHANCED STATIC ANALYSIS CONFIGURATION
; =============================================================================
[env:static-analysis-enhanced]
extends = env:esp32dev
check_tool = 
    clang-tidy
    clang-format
    cppcheck
check_flags =
    clang-tidy: >
        --checks=bugprone-*,misc-*,performance-*,clang-analyzer-*,
        modernize-use-trailing-return-type,
        readability-identifier-naming,
        -bugprone-easily-swappable-parameters,
        -readability-convert-member-functions-to-static
    clang-format: --style=file -i
    cppcheck: >
        --enable=warning,style,performance,portability,unusedFunction
        --language=c++
        --std=c++17
        --platform=unspecified
        --error-exitcode=3
        --verbose

build_flags = 
    -D ESP32
    -D ARDUINO_ARCH_ESP32
    -D CONFIG_FREERTOS_HZ=1000
    -D ARDUINO_LOOP_STACK_SIZE=8192
    -std=gnu++17
    -fstack-protector-all
"@
    
    # Add configuration to the end of the file
    Add-Content -Path "platformio.ini" -Value $platformioConfig
    Write-Host "OK: platformio.ini updated with enhanced analysis" -ForegroundColor Green
}

function Create-CompileCommands {
    Write-Host "Creating compile_commands.json..." -ForegroundColor Cyan
    
    try {
        pio run -e esp32dev --target compiledb
        if (Test-Path "compile_commands.json") {
            Write-Host "OK: compile_commands.json created" -ForegroundColor Green
        } else {
            Write-Host "WARNING: compile_commands.json not created, but this is not critical" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "WARNING: Could not create compile_commands.json" -ForegroundColor Yellow
    }
}

function Test-Installation {
    Write-Host "Testing installation..." -ForegroundColor Cyan
    
    $tools = @("clangd", "clang-tidy", "clang-format")
    $allInstalled = $true
    
    foreach ($tool in $tools) {
        if (Test-Command $tool) {
            $version = & $tool --version 2>$null | Select-Object -First 1
            Write-Host "OK: $tool installed - $version" -ForegroundColor Green
        } else {
            Write-Host "ERROR: $tool not found" -ForegroundColor Red
            $allInstalled = $false
        }
    }
    
    if ($allInstalled) {
        Write-Host "SUCCESS: All Clang tools installed and configured!" -ForegroundColor Green
    } else {
        Write-Host "WARNING: Some tools not installed. Check manual installation." -ForegroundColor Yellow
    }
}

# Main script logic
Write-Host "Clang Tools Setup for JXCT Soil Sensor v3.10.1" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

if ($All -or $InstallTools) {
    Install-ClangTools
}

if ($All -or $UpdateConfigs) {
    Update-ClangConfigs
    Update-PlatformIOConfig
}

if ($All -or $SetupLSP) {
    Setup-LSP
    Create-CompileCommands
}

if ($All) {
    Test-Installation
}

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Restart Cursor/VS Code to apply LSP settings" -ForegroundColor White
Write-Host "2. Run: pio check -e static-analysis-enhanced" -ForegroundColor White
Write-Host "3. Check autocompletion in editor" -ForegroundColor White
Write-Host "4. Run: python scripts/run_clang_tidy_analysis.py" -ForegroundColor White
Write-Host "==================================================" -ForegroundColor Cyan 