const vscode = require('vscode');
const path = require('path');
const fs = require('fs');
const {
    parseNyotaColorToken,
    toNyotaHex,
    exactNamedColor
} = require('./color-palette');

const COLOR_TOKEN_RE = /0[xX][0-9A-Fa-f]{8}(?![0-9A-Za-z_])|0[xX][0-9A-Fa-f]{6}(?![0-9A-Za-z_])|\b[A-Z][A-Z0-9]*\b/g;

function maskStringsAndComments(line) {
    const chars = Array.from(line);
    let inString = false;

    for (let i = 0; i < chars.length; i++) {
        const ch = chars[i];

        if (!inString && ch === '#') {
            for (let j = i; j < chars.length; j++) {
                chars[j] = ' ';
            }
            break;
        }

        if (ch === '"' && (i === 0 || line[i - 1] !== '\\')) {
            inString = !inString;
            chars[i] = ' ';
            continue;
        }

        if (inString) {
            chars[i] = ' ';
        }
    }

    return chars.join('');
}

function colorToVscode(color) {
    return new vscode.Color(
        color.r / 255,
        color.g / 255,
        color.b / 255,
        color.a / 255
    );
}

function componentToByte(value) {
    return Math.max(0, Math.min(255, Math.round(value * 255)));
}

function colorPresentation(label, range) {
    const presentation = new vscode.ColorPresentation(label);
    presentation.textEdit = vscode.TextEdit.replace(range, label);
    return presentation;
}

const nyotaColorProvider = {
    provideDocumentColors(document) {
        const colors = [];

        for (let lineNumber = 0; lineNumber < document.lineCount; lineNumber++) {
            const original = document.lineAt(lineNumber).text;
            const code = maskStringsAndComments(original);
            COLOR_TOKEN_RE.lastIndex = 0;

            let match;
            while ((match = COLOR_TOKEN_RE.exec(code)) !== null) {
                const token = original.slice(match.index, match.index + match[0].length);
                const parsed = parseNyotaColorToken(token);

                // BACKDROP zalezy od sceny pod obiektem, wiec nie ma jednego
                // koloru, ktory VS Code moglby pokazac w probce.
                if (!parsed || parsed.mode === 'backdrop') {
                    continue;
                }

                const range = new vscode.Range(
                    lineNumber,
                    match.index,
                    lineNumber,
                    match.index + match[0].length
                );
                colors.push(new vscode.ColorInformation(range, colorToVscode(parsed)));
            }
        }

        return colors;
    },

    provideColorPresentations(color, context) {
        const r = componentToByte(color.red);
        const g = componentToByte(color.green);
        const b = componentToByte(color.blue);
        const a = componentToByte(color.alpha);
        const presentations = [];

        if (a === 0) {
            presentations.push(colorPresentation('TRANSPARENT', context.range));
        } else {
            const named = exactNamedColor(r, g, b, a);
            if (named) {
                presentations.push(colorPresentation(named, context.range));
            }
        }

        presentations.push(
            colorPresentation(toNyotaHex(r, g, b, a), context.range)
        );

        return presentations;
    }
};

function activate(context) {
    const runCurrentFile = vscode.commands.registerCommand(
        'nyota.runCurrentFile',
        async () => {
            const editor = vscode.window.activeTextEditor;

            if (!editor) {
                vscode.window.showErrorMessage('Nyota: brak otwartego pliku.');
                return;
            }

            const document = editor.document;

            if (document.languageId !== 'nyota') {
                vscode.window.showErrorMessage('Nyota: aktualny plik nie jest plikiem .nyo.');
                return;
            }

            if (document.isUntitled || document.uri.scheme !== 'file') {
                vscode.window.showErrorMessage('Nyota: zapisz plik na dysku przed uruchomieniem.');
                return;
            }

            if (document.isDirty) {
                const saved = await document.save();
                if (!saved) {
                    vscode.window.showErrorMessage('Nyota: nie udało się zapisać pliku.');
                    return;
                }
            }

            const filePath = document.uri.fsPath;
            const cwd = path.dirname(filePath);
            const config = vscode.workspace.getConfiguration('nyota');
            const interpreter = String(config.get('interpreterPath', 'nyota')).trim() || 'nyota';

            if (path.isAbsolute(interpreter) && !fs.existsSync(interpreter)) {
                vscode.window.showErrorMessage(
                    `Nyota: interpreter nie istnieje: ${interpreter}`
                );
                return;
            }

            const execution = new vscode.ProcessExecution(
                interpreter,
                [filePath],
                { cwd }
            );

            const task = new vscode.Task(
                {
                    type: 'nyota',
                    file: filePath
                },
                vscode.TaskScope.Global,
                `Run ${path.basename(filePath)}`,
                'Nyota',
                execution,
                []
            );

            task.presentationOptions = {
                reveal: vscode.TaskRevealKind.Always,
                echo: true,
                focus: true,
                panel: vscode.TaskPanelKind.Dedicated,
                showReuseMessage: false,
                clear: true
            };

            try {
                await vscode.tasks.executeTask(task);
            } catch (error) {
                const message = error instanceof Error ? error.message : String(error);
                vscode.window.showErrorMessage(`Nyota: nie udało się uruchomić programu: ${message}`);
            }
        }
    );

    const colorProvider = vscode.languages.registerColorProvider(
        { language: 'nyota' },
        nyotaColorProvider
    );

    context.subscriptions.push(runCurrentFile, colorProvider);
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
};
