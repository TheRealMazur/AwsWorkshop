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
#include <aws/dynamodb/model/ScanRequest.h>
#include <aws/lambda-runtime/runtime.h>

static constexpr char TAG[] = "lambda";
static constexpr Aws::Utils::Logging::LogLevel globalLogLevel =
    Aws::Utils::Logging::LogLevel::Off;

using aws::lambda_runtime::invocation_response;
using Aws::Utils::Json::JsonValue;

class ProductHandler {
   public:
    ProductHandler() = default;
    invocation_response handler() { return getProduct(); }

   private:
    invocation_response getProduct() {
        Aws::Client::ClientConfiguration config;
        config.region = Aws::Environment::GetEnv("AWS_REGION");
        config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";
        config.disableExpectHeader = true;

        auto credentialsProvider =
            Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
        Aws::DynamoDB::DynamoDBClient client(credentialsProvider, config);
        Aws::DynamoDB::Model::ScanRequest request;
        request.SetTableName("ProductCatalog");

        auto outcome = client.Scan(request);

        if (outcome.IsSuccess()) {
            const auto& items = outcome.GetResult().GetItems();
            Aws::Utils::Array<JsonValue> array(items.size());
            for (unsigned int i = 0; i < items.size(); ++i) {
                JsonValue body;
                body.WithString("description",
                                items[i].find("description")->second.GetS());
                body.WithString("id", items[i].find("id")->second.GetS());
                body.WithString("name", items[i].find("name")->second.GetS());
                array[i] = body;
            }
            return aws::lambda_runtime::invocation_response::success(
                JsonValue().AsArray(array).View().WriteCompact(),
                "application/json");
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

invocation_response myHandler(
    aws::lambda_runtime::invocation_request const& req) {
    AWS_LOGSTREAM_DEBUG(TAG, "received payload: " << req.payload);
    ProductHandler productHandler;
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
