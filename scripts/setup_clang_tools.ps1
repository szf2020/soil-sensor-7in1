#!/usr/bin/env pwsh
# =============================================================================
# 🔧 СКРИПТ НАСТРОЙКИ ИНСТРУМЕНТОВ CLANG - JXCT Soil Sensor
# =============================================================================
# Автор: JXCT Development Team
# Версия: 1.0.0
# Дата: 27.07.2025
# Описание: Автоматическая установка и настройка инструментов Clang для ESP32

param(
    [switch]$InstallTools,
    [switch]$UpdateConfigs,
    [switch]$SetupLSP,
    [switch]$All
)

# Цвета для вывода
$Colors = @{
    Info = "Cyan"
    Success = "Green"
    Warning = "Yellow"
    Error = "Red"
}

function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Colors[$Color]
}

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
    Write-ColorOutput "🔧 Установка инструментов Clang..." "Info"
    
    $tools = @{
        "clangd" = "LLVM.LLVM"
        "clang-tidy" = "LLVM.LLVM"
        "clang-format" = "LLVM.LLVM"
        "scan-build" = "LLVM.LLVM"
    }
    
    foreach ($tool in $tools.Keys) {
        Write-ColorOutput "Проверка $tool..." "Info"
        if (Test-Command $tool) {
            Write-ColorOutput "✅ $tool уже установлен" "Success"
        } else {
            Write-ColorOutput "❌ $tool не найден, установка..." "Warning"
            
            # Попытка установки через winget
            try {
                Write-ColorOutput "Установка через winget..." "Info"
                winget install $tools[$tool] --accept-source-agreements --accept-package-agreements
                if (Test-Command $tool) {
                    Write-ColorOutput "✅ $tool успешно установлен" "Success"
                } else {
                    Write-ColorOutput "❌ Не удалось установить $tool" "Error"
                }
            } catch {
                Write-ColorOutput "❌ Ошибка установки $tool: $($_.Exception.Message)" "Error"
            }
        }
    }
}

function Update-ClangConfigs {
    Write-ColorOutput "📝 Обновление конфигураций Clang..." "Info"
    
    # Обновление .clang-format
    $clangFormatConfig = @"
BasedOnStyle: Google
Language: Cpp
AccessModifierOffset: -2
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
AlignEscapedNewlines: Left
AlignOperands: true
AlignTrailingComments: true
AllowAllParametersOfDeclarationOnNextLine: true
AllowShortBlocksOnASingleLine: false
AllowShortCaseLabelsOnASingleLine: false
AllowShortFunctionsOnASingleLine: Empty
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
AlwaysBreakAfterReturnType: None
AlwaysBreakBeforeMultilineStrings: true
AlwaysBreakTemplateDeclarations: Yes
BinPackArguments: true
BinPackParameters: true
BraceWrapping:
  AfterClass: false
  AfterControlStatement: false
  AfterEnum: false
  AfterFunction: false
  AfterNamespace: false
  AfterStruct: false
  AfterUnion: false
  BeforeCatch: false
  BeforeElse: false
  IndentBraces: false
BreakBeforeBinaryOperators: None
BreakBeforeBraces: Allman
BreakBeforeTernaryOperators: true
BreakStringLiterals: true
ColumnLimit: 120
CommentPragmas: '^ IWYU pragma:'
ConstructorInitializerAllOnOneLineOrOnePerLine: true
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
Cpp11BracedListStyle: true
DerivePointerAlignment: false
DisableFormat: false
ExperimentalAutoDetectBinPacking: false
FixNamespaceComments: true
IncludeBlocks: Preserve
IndentCaseLabels: true
IndentPPDirectives: None
IndentWidth: 4
IndentWrappedFunctionNames: false
KeepEmptyLinesAtTheStartOfBlocks: false
MaxEmptyLinesToKeep: 1
NamespaceIndentation: None
PenaltyBreakAssignment: 2
PenaltyBreakBeforeFirstCallParameter: 1
PenaltyBreakComment: 300
PenaltyBreakFirstLessLess: 120
PenaltyBreakString: 1000
PenaltyExcessCharacter: 1000000
PenaltyReturnTypeOnItsOwnLine: 200
PointerAlignment: Left
ReflowComments: true
SortIncludes: true
SortUsingDeclarations: true
SpaceAfterCStyleCast: false
SpaceAfterTemplateKeyword: true
SpaceBeforeAssignmentOperators: true
SpaceInEmptyParentheses: false
SpacesInAngles: false
SpacesInContainerLiterals: false
SpacesInCStyleCastParentheses: false
SpacesInParentheses: false
SpacesInSquareBrackets: false
Standard: Cpp11
TabWidth: 4
UseTab: Never

# ESP32 специфика
ForEachMacros:
  - 'STATEMENT_FOR_EACH_ITEM'
  - 'FOR_EACH_SENSOR'
"@
    
    $clangFormatConfig | Out-File -FilePath ".clang-format" -Encoding UTF8
    Write-ColorOutput "✅ .clang-format обновлен" "Success"
    
    # Создание .clang-tidy
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
  - key:   misc-non-private-member-variables-in-classes.IgnoreClassesWithAllMemberVariablesBeingPublic
    value: 'true'
  - key:   readability-identifier-naming.ClassCase
    value: CamelCase
  - key:   readability-identifier-naming.FunctionCase
    value: lower_case
  - key:   readability-identifier-naming.VariableCase
    value: lower_case
  - key:   readability-identifier-naming.ConstantCase
    value: UPPER_CASE
  - key:   readability-identifier-naming.ParameterCase
    value: lower_case
  - key:   readability-identifier-naming.MacroDefinitionCase
    value: UPPER_CASE

# ESP32 специфичные настройки
SystemHeaders: false
HeaderFilterRegex: '^(include|src)/.*\.(h|hpp|cpp)$'
"@
    
    $clangTidyConfig | Out-File -FilePath ".clang-tidy" -Encoding UTF8
    Write-ColorOutput "✅ .clang-tidy создан" "Success"
}

function Setup-LSP {
    Write-ColorOutput "🔧 Настройка LSP для Cursor/VS Code..." "Info"
    
    # Создание .vscode/settings.json с настройками clangd
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
        "github.copilot.enable" = @{
            "*" = $true
            "plaintext" = $false
            "markdown" = $true
        }
        "editor.inlineSuggest.enabled" = $true
        "editor.suggest.showKeywords" = $true
        "editor.suggest.showSnippets" = $true
        "files.exclude" = @{
            "**/.pio" = $false
            "**/build" = $false
            "**/docs" = $false
        }
        "search.exclude" = @{
            "**/build" = $true
            "**/.pio" = $true
            "**/docs" = $true
            "**/site" = $true
            "**/__pycache__" = $true
        }
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
    
    # Создание директории .vscode если не существует
    if (!(Test-Path ".vscode")) {
        New-Item -ItemType Directory -Path ".vscode" | Out-Null
    }
    
    $vscodeSettings | ConvertTo-Json -Depth 10 | Out-File -FilePath ".vscode/settings.json" -Encoding UTF8
    Write-ColorOutput "✅ .vscode/settings.json обновлен с настройками clangd" "Success"
}

function Update-PlatformIOConfig {
    Write-ColorOutput "📝 Обновление platformio.ini..." "Info"
    
    # Добавление новой секции для расширенного анализа
    $platformioConfig = @"

; =============================================================================
; 🔍 ENHANCED STATIC ANALYSIS CONFIGURATION - Расширенный статический анализ
; =============================================================================
[env:static-analysis-enhanced]
extends = env:esp32dev
check_tool = 
    clang-tidy
    clang-format
    cppcheck
    clang-static-analyzer
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
    clang-static-analyzer: --analyze

; ESP32 специфичные флаги
build_flags = 
    -D ESP32
    -D ARDUINO_ARCH_ESP32
    -D CONFIG_FREERTOS_HZ=1000
    -D ARDUINO_LOOP_STACK_SIZE=8192
    -std=gnu++17
    -fstack-protector-all
"@
    
    # Добавление конфигурации в конец файла
    Add-Content -Path "platformio.ini" -Value $platformioConfig
    Write-ColorOutput "✅ platformio.ini обновлен с расширенным анализом" "Success"
}

function Create-CompileCommands {
    Write-ColorOutput "📝 Создание compile_commands.json..." "Info"
    
    try {
        # Создание compile_commands.json для LSP
        pio run -e esp32dev --target compiledb
        if (Test-Path "compile_commands.json") {
            Write-ColorOutput "✅ compile_commands.json создан" "Success"
        } else {
            Write-ColorOutput "⚠️ compile_commands.json не создан, но это не критично" "Warning"
        }
    } catch {
        Write-ColorOutput "⚠️ Не удалось создать compile_commands.json: $($_.Exception.Message)" "Warning"
    }
}

function Test-Installation {
    Write-ColorOutput "🧪 Тестирование установки..." "Info"
    
    $tools = @("clangd", "clang-tidy", "clang-format")
    $allInstalled = $true
    
    foreach ($tool in $tools) {
        if (Test-Command $tool) {
            $version = & $tool --version 2>$null | Select-Object -First 1
            Write-ColorOutput "✅ $tool установлен: $version" "Success"
        } else {
            Write-ColorOutput "❌ $tool не найден" "Error"
            $allInstalled = $false
        }
    }
    
    if ($allInstalled) {
        Write-ColorOutput "🎉 Все инструменты Clang успешно установлены и настроены!" "Success"
    } else {
        Write-ColorOutput "⚠️ Некоторые инструменты не установлены. Проверьте установку вручную." "Warning"
    }
}

# Основная логика скрипта
Write-ColorOutput "🔧 Настройка инструментов Clang для JXCT Soil Sensor v3.10.1" "Info"
Write-ColorOutput "==================================================================" "Info"

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

Write-ColorOutput "==================================================================" "Info"
Write-ColorOutput "📋 Следующие шаги:" "Info"
Write-ColorOutput "1. Перезапустите Cursor/VS Code для применения настроек LSP" "Info"
Write-ColorOutput "2. Запустите: pio check -e static-analysis-enhanced" "Info"
Write-ColorOutput "3. Проверьте автодополнение в редакторе" "Info"
Write-ColorOutput "4. Запустите: python scripts/run_clang_tidy_analysis.py" "Info"
Write-ColorOutput "==================================================================" "Info" 