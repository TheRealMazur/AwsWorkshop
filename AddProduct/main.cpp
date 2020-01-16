#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/UUID.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <aws/dynamodb/model/PutItemRequest.h>
#include <aws/lambda-runtime/runtime.h>

static char const TAG[] = "lambda";
static Aws::Utils::Logging::LogLevel globalLogLevel = Aws::Utils::Logging::LogLevel::Off;

using aws::lambda_runtime::invocation_response;
using Aws::Utils::Json::JsonValue;

class ProductHandler {
 public:
  ProductHandler(Aws::String name, Aws::String description)
      : mName(name),
        mDescription(description),
        mId(Aws::Utils::UUID::RandomUUID()) {}

  invocation_response handler() { return addProduct(); }

 private:
  invocation_response addProduct() {
    Aws::Client::ClientConfiguration config;
    config.region = Aws::Environment::GetEnv("AWS_REGION");
    config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";

    auto credentialsProvider =
        Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
    Aws::DynamoDB::DynamoDBClient client(credentialsProvider, config);
    Aws::DynamoDB::Model::PutItemRequest request;
    request.SetTableName("ProductCatalog");
    request.SetItem(Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>{
        {"id", Aws::DynamoDB::Model::AttributeValue(mId)},
        {"name", Aws::DynamoDB::Model::AttributeValue(mName)},
        {"description", Aws::DynamoDB::Model::AttributeValue(mDescription)}});
    auto outcome = client.PutItem(request);
    if (outcome.IsSuccess()) {
      JsonValue body;
      body.WithString("id", mId)
          .WithString("name", mName)
          .WithString("description", mDescription);
      JsonValue response;
      response.WithString("body", body.View().WriteCompact())
          .WithInteger("statusCode", 200);
      return aws::lambda_runtime::invocation_response::success(
          response.View().WriteCompact(), "application/json");
    } else {
      JsonValue response;
      response.WithString("body", "Internal Server Error")
          .WithInteger("statusCode", 500);
      return aws::lambda_runtime::invocation_response::failure(
          response.View().WriteCompact(), "application/json");
    }
    AWS_LOGSTREAM_ERROR(TAG, "database query failed: " << outcome.GetError());
  }

  Aws::String mName;
  Aws::String mDescription;
  Aws::String mId;
};

bool validateInput(const JsonValue& eventJson) {
  return (eventJson.WasParseSuccessful() &&
          eventJson.View().ValueExists("name") &&
          eventJson.View().ValueExists("desc"));
}

invocation_response myHandler(
    aws::lambda_runtime::invocation_request const& req) {
  AWS_LOGSTREAM_DEBUG(TAG, "received payload: " << req.payload);
  JsonValue eventJson(req.payload);
  if (!validateInput(eventJson)) {
    JsonValue response;
    response.WithString("body", "Error in input, please verify payload body")
        .WithInteger("statusCode", 400);
    auto const apig_response = response.View().WriteCompact();
    AWS_LOGSTREAM_ERROR(TAG, "Validation failed. " << apig_response);
    return aws::lambda_runtime::invocation_response::failure(
        apig_response, "application/json");
  }
  auto eventJsonView = eventJson.View();
  ProductHandler productHandler(eventJsonView.GetString("name"),
                                eventJsonView.GetString("desc"));
  return productHandler.handler();
}
std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()>
GetConsoleLoggerFactory() {
  return [] {
    return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
        "console_logger", globalLogLevel);
  };
}

int main() {
  Aws::SDKOptions options;
  options.loggingOptions.logLevel = globalLogLevel;
  options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
  InitAPI(options);
  run_handler(myHandler);
  ShutdownAPI(options);
  return 0;
}
