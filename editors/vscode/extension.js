const vscode = require('vscode');
const path = require('path');
const fs = require('fs');

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

    context.subscriptions.push(runCurrentFile);
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
};
