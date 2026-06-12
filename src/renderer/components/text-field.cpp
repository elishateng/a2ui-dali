#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderTextField(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  FlexLayout container = FlexLayout::New();
  container.SetDirection(FlexDirection::COLUMN);
  container.SetRequestedWidth(MATCH_PARENT);
  container.SetRequestedHeight(WRAP_CONTENT);
  container.SetMargin(Extents(0, 0, 4, 4));

  // Label
  const char* labelText = GetNodeString(*comp.rawNode, "label", "");
  if(labelText[0] != '\0')
  {
    Label label = Label::New(labelText);
    label.SetFontSize(Metrics::FontLabel());
    label.SetTextColor(A2uiTheme::Color("OnSurfaceContainerLow"));  // OneUIColor.OnSurfaceContainerLow
    label.SetRequestedWidth(MATCH_PARENT);
    label.SetRequestedHeight(WRAP_CONTENT);
    label.SetMargin(Extents(0, 0, 0, 4));
    container.Add(label);
  }

  // Bound path for two-way binding
  const TreeNode* valueNode = comp.rawNode->Find("value");
  std::string boundPath = valueNode ? GetBoundPath(valueNode, ctx) : "";

  const char* placeholder = GetNodeString(*comp.rawNode, "placeholder", "");
  std::string currentValue;
  if(!boundPath.empty())
  {
    currentValue = ctx.GetDataModel().GetString(boundPath);
  }

  // Real InputField for text input
  InputField inputField = InputField::New();
  inputField.SetPlaceholder(placeholder);
  inputField.SetPlaceholderColor(A2uiTheme::Color("OnSurfaceContainerLow"));  // neutral placeholder grey
  inputField.SetFontSize(Metrics::FontInput());
  inputField.SetTextColor(COLOR_TEXT_DEFAULT);
  inputField.SetCursorColor(COLOR_TEXT_DEFAULT);
  inputField.SetCursorWidth(2);
  inputField.SetSelectionColor(A2uiTheme::Color("Primary"));
  inputField.SetRequestedWidth(MATCH_PARENT);
  inputField.SetRequestedHeight(WRAP_CONTENT);
  inputField.SetPadding(Extents(12, 12, 10, 10));
  inputField.SetBackgroundColor(COLOR_INPUT_BG);
  inputField.SetCornerRadius(Metrics::RadiusInput());
  inputField.SetBorderlineWidth(Metrics::BorderInput());                 // OneUI outlined input box
  inputField.SetBorderlineColor(COLOR_INPUT_BORDER);   // (was missing → no visible box)

  if(!currentValue.empty())
  {
    inputField.SetText(Dali::String(currentValue.c_str()));
  }

  // Two-way binding: InputField → DataModel
  if(!boundPath.empty())
  {
    inputField.TextChangedSignal().Connect(this,
      [boundPath, ctx](View changedView) mutable {
        InputField field = InputField::DownCast(changedView);
        if(field)
        {
          std::string val = field.GetText().CStr();
          ctx.GetDataModel().SetValue(boundPath, val);
        }
      });

    // DataModel → InputField (for server-side updates)
    ctx.GetDataModel().Watch(boundPath,
      [inputField](const std::string&, const std::string& val) mutable {
        std::string current = inputField.GetText().CStr();
        if(current != val)
        {
          inputField.SetText(Dali::String(val.c_str()));
        }
      });
  }

  container.Add(inputField);

  // Error label (initially hidden)
  Label errorLabel = Label::New("");
  errorLabel.SetFontSize(Metrics::FontError());
  errorLabel.SetTextColor(COLOR_ERROR);
  errorLabel.SetRequestedWidth(MATCH_PARENT);
  errorLabel.SetRequestedHeight(WRAP_CONTENT);
  errorLabel.SetProperty(Actor::Property::VISIBLE, false);
  errorLabel.SetMargin(Extents(0, 0, 2, 0));
  container.Add(errorLabel);

  // Setup checks validation
  if(!boundPath.empty())
  {
    SetupChecks(comp, ctx, errorLabel, boundPath);
  }

  return container;
}
} // namespace A2ui
