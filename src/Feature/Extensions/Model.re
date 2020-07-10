open Oni_Core;
open Exthost.Extension;

[@deriving show({with_path: false})]
type msg =
  | Activated(string /* id */)
  | Discovered([@opaque] list(Scanner.ScanResult.t))
  | ExecuteCommand({
      command: string,
      arguments: [@opaque] list(Json.t),
    })
  | KeyPressed(string)
  | SearchQueryResults(Service_Extensions.Query.t)
  | SearchQueryError(string)
  | SearchText(Feature_InputText.msg)
  | UninstallExtensionClicked({extensionId: string})
  | UninstallExtensionSuccess({extensionId: string})
  | UninstallExtensionFailed({
      extensionId: string,
      errorMsg: string,
    });

type outmsg =
  | Nothing
  | Focus
  | Effect(Isolinear.Effect.t(msg));

type model = {
  activatedIds: list(string),
  extensions: list(Scanner.ScanResult.t),
  searchText: Feature_InputText.model,
  latestQuery: option(Service_Extensions.Query.t),
  extensionsFolder: option(string),
};

let initial = (~extensionsFolder) => {
  activatedIds: [],
  extensions: [],
  searchText: Feature_InputText.create(~placeholder="Type to search..."),
  latestQuery: None,
  extensionsFolder,
};

let searchResults = ({latestQuery, _}) =>
  switch (latestQuery) {
  | None => []
  | Some(query) => query |> Service_Extensions.Query.results
  };

module Internal = {
  let filterBundled = (scanner: Scanner.ScanResult.t) => {
    let name = scanner.manifest |> Manifest.identifier;

    name == "vscode.typescript-language-features"
    || name == "vscode.css-language-features"
    || name == "jaredkent.laserwave"
    || name == "jaredly.reason-vscode"
    || name == "arcticicestudio.nord-visual-studio-code";
  };
  let markActivated = (id: string, model) => {
    ...model,
    activatedIds: [id, ...model.activatedIds],
  };

  let add = (extensions, model) => {
    ...model,
    extensions: extensions @ model.extensions,
  };
};

let getExtensions = (~category, model) => {
  let results =
    model.extensions
    |> List.filter((ext: Scanner.ScanResult.t) => ext.category == category);

  switch (category) {
  | Scanner.Bundled => List.filter(Internal.filterBundled, results)
  | _ => results
  };
};

let checkAndUpdateSearchText = (~previousText, ~newText, ~query) =>
  if (previousText != newText) {
    if (String.length(newText) == 0) {
      None;
    } else {
      Some(Service_Extensions.Query.create(~searchText=newText));
    };
  } else {
    query;
  };

let update = (~extHostClient, msg, model) => {
  switch (msg) {
  | Activated(id) => (Internal.markActivated(id, model), Nothing)
  | Discovered(extensions) => (Internal.add(extensions, model), Nothing)
  | ExecuteCommand({command, arguments}) => (
      model,
      Effect(
        Service_Exthost.Effects.Commands.executeContributedCommand(
          ~command,
          ~arguments,
          extHostClient,
        ),
      ),
    )
  | KeyPressed(key) =>
    let previousText = model.searchText |> Feature_InputText.value;
    let searchText' = Feature_InputText.handleInput(~key, model.searchText);
    let newText = searchText' |> Feature_InputText.value;
    let latestQuery =
      checkAndUpdateSearchText(
        ~previousText,
        ~newText,
        ~query=model.latestQuery,
      );
    ({...model, searchText: searchText', latestQuery}, Nothing);
  | SearchText(msg) =>
    let previousText = model.searchText |> Feature_InputText.value;
    let searchText' = Feature_InputText.update(msg, model.searchText);
    let newText = searchText' |> Feature_InputText.value;
    let latestQuery =
      checkAndUpdateSearchText(
        ~previousText,
        ~newText,
        ~query=model.latestQuery,
      );
    ({...model, searchText: searchText', latestQuery}, Focus);
  | SearchQueryResults(queryResults) => (
      {...model, latestQuery: Some(queryResults)},
      Nothing,
    )
  | SearchQueryError(_queryResults) =>
    // TODO: Error experience?
    ({...model, latestQuery: None}, Nothing)
  | UninstallExtensionClicked({extensionId}) =>
    let toMsg = (
      fun
      | Ok () => UninstallExtensionSuccess({extensionId: extensionId})
      | Error(msg) => UninstallExtensionFailed({extensionId, errorMsg: msg})
    );
    let eff =
      Service_Extensions.Effects.uninstall(
        ~extensionsFolder=model.extensionsFolder,
        ~toMsg,
        extensionId,
      );
    (model, Effect(eff));
  | UninstallExtensionSuccess(_)
  | UninstallExtensionFailed(_) =>
    // TODO: Error / success experience
    (model, Nothing)
  };
};
