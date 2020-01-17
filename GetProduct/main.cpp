#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/lambda-runtime/runtime.h>

static constexpr char TAG[] = "lambda";
static constexpr Aws::Utils::Logging::LogLevel globalLogLevel =
    Aws::Utils::Logging::LogLevel::Off;

using aws::lambda_runtime::invocation_response;
using Aws::Utils::Json::JsonValue;

class ProductHandler {
   public:
    ProductHandler() = default;
    invocation_response handler(Aws::String id) { return getProduct(id); }

   private:
    invocation_response getProduct(Aws::String id) {
        Aws::Client::ClientConfiguration config;
        config.region = Aws::Environment::GetEnv("AWS_REGION");
        config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";
        config.disableExpectHeader = true;

        auto credentialsProvider =
            Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
        Aws::DynamoDB::DynamoDBClient client(credentialsProvider, config);
        Aws::DynamoDB::Model::GetItemRequest request;
        request.SetTableName("ProductCatalog");
        request.AddKey("id", Aws::DynamoDB::Model::AttributeValue(id));

        auto outcome = client.GetItem(request);

        if (outcome.IsSuccess()) {
            const auto& item = outcome.GetResult().GetItem();
            if (!item.empty()) {
                JsonValue body;
                body.WithString("description",
                                item.find("description")->second.GetS());
                body.WithString("id", item.find("id")->second.GetS());
                body.WithString("name", item.find("name")->second.GetS());
                JsonValue response;
                response.WithString("body", body.View().WriteCompact())
                    .WithInteger("statusCode", 200);
                return aws::lambda_runtime::invocation_response::success(
                    response.View().WriteCompact(), "application/json");
            } else {
                JsonValue response;
                response.WithString("body", "Resource not found")
                    .WithInteger("statusCode", 404);
                return aws::lambda_runtime::invocation_response::failure(
                    response.View().WriteCompact(), "application/json");
            }
        } else {
            JsonValue response;
            response.WithString("body", "Internal Server Error")
                .WithInteger("statusCode", 500);
            return aws::lambda_runtime::invocation_response::failure(
                response.View().WriteCompact(), "application/json");
        }
        AWS_LOGSTREAM_ERROR(TAG,
                            "database query failed: " << outcome.GetError());
    }
};

bool validateInput(const JsonValue& eventJson) {
    return (eventJson.WasParseSuccessful() &&
            eventJson.View().ValueExists("pathParameters") &&
            eventJson.View().GetObject("pathParameters").ValueExists("id"));
}

invocation_response myHandler(
    aws::lambda_runtime::invocation_request const& req) {
    AWS_LOGSTREAM_DEBUG(TAG, "received payload: " << req.payload);
    JsonValue eventJson(req.payload);
    if (!validateInput(eventJson)) {
        JsonValue response;
        response
            .WithString("body", "Error in input, please verify payload body")
            .WithInteger("statusCode", 400);
        auto const responseView = response.View().WriteCompact();
        AWS_LOGSTREAM_ERROR(TAG, "Validation failed. " << responseView);
        return aws::lambda_runtime::invocation_response::failure(
            responseView, "application/json");
    }
    auto eventJsonView = eventJson.View();
    ProductHandler productHandler;
    return productHandler.handler(
        eventJsonView.GetObject("pathParameters").GetString("id"));
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
