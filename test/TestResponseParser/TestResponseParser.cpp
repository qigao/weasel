#include <ResponseParser.h>
#include <boost/archive/text_woarchive.hpp>
#include <sstream>
#include <string>
// TinyTest's assertion macros must follow Boost's declarations.
#include <tinytest.hpp>

namespace {
bool Parse(weasel::ResponseParser& parser, std::wstring response) {
  return parser(response.data(), static_cast<UINT>(response.size()));
}

// Match the current Server's ctx.cand payload, including archive metadata.
std::wstring Candidates(const weasel::CandidateInfo& candidates) {
  std::wstringstream stream;
  {
    boost::archive::text_woarchive archive(stream);
    archive << candidates;
  }
  return L"ctx.cand=" + stream.str() + L"\n";
}
}  // namespace

spec("Weasel response parser") {
  group("original response samples") {
    it("leaves a noop response empty") {
      std::wstring commit;
      weasel::Context context;
      weasel::Status status;
      weasel::ResponseParser parser(&commit, &context, &status);
      check(Parse(parser, L"action=noop\n.\n"));
      check_empty(commit);
      check(context.empty());
    }

    it("commits Chinese text containing an equals sign without clearing context") {
      std::wstring commit;
      weasel::Context context;
      weasel::Status status;
      context.aux.str = L"從前的值";
      weasel::ResponseParser parser(&commit, &context, &status);
      check(Parse(parser, L"action=commit\ncommit=教這句話上屏=3.14\n.\n"));
      check(commit == L"教這句話上屏=3.14");
      check(context.preedit.empty());
      check(context.aux.str == L"從前的值");
      check_empty(context.cinfo.candies);
    }

    it("updates preedit and auxiliary text without committing") {
      std::wstring commit;
      weasel::Context context;
      weasel::Status status;
      weasel::ResponseParser parser(&commit, &context, &status);
      check(Parse(parser, L"action=ctx\nctx.preedit=寫作串=3.14\n"
                          L"ctx.aux=sie'zuoh'chuan=3.14\n.\n"));
      check_empty(commit);
      check(context.preedit.str == L"寫作串=3.14");
      check_empty(context.preedit.attributes);
      check(context.aux.str == L"sie'zuoh'chuan=3.14");
    }

    it("updates two candidates, selection, paging and preedit highlighting") {
      std::wstring commit;
      weasel::Context context;
      weasel::Status status;
      weasel::ResponseParser parser(&commit, &context, &status);
      weasel::CandidateInfo candidates;
      candidates.candies = {weasel::Text(L"候選甲"), weasel::Text(L"候選乙")};
      candidates.highlighted = 1;
      candidates.currentPage = 0;
      candidates.totalPages = 1;
      const std::wstring response =
          L"action=commit,ctx\nctx.preedit=候選乙=3.14\n"
          L"ctx.preedit.cursor=0,3\n" + Candidates(candidates) + L".\n";
      check(Parse(parser, response));
      check_empty(commit);
      check(context.preedit.str == L"候選乙=3.14");
      check_size(context.preedit.attributes, 1u);
      const auto& attribute = context.preedit.attributes.front();
      check_equal(attribute.type, weasel::HIGHLIGHTED);
      check_equal(attribute.range.start, 0);
      check_equal(attribute.range.end, 3);
      check(context.aux.empty());
      check_size(context.cinfo.candies, 2u);
      check(context.cinfo.candies[0].str == L"候選甲");
      check(context.cinfo.candies[1].str == L"候選乙");
      check_equal(context.cinfo.highlighted, 1);
      check_equal(context.cinfo.currentPage, 0);
      check_equal(context.cinfo.totalPages, 1);
    }
  }

  group("text and candidate data") {
    it("preserves CJK and UTF-16 surrogate pairs in commit and preedit") {
      std::wstring commit;
      weasel::Context context;
      weasel::ResponseParser parser(&commit, &context);
      check(Parse(parser, L"action=commit,ctx\ncommit=中文\U0001F600\n"
                          L"ctx.preedit=輸入\U0001F680\n.\n"));
      check(commit == L"中文\U0001F600");
      check(context.preedit.str == L"輸入\U0001F680");
    }

    it("unescapes newlines, tabs and backslashes in text") {
      std::wstring commit;
      weasel::Context context;
      weasel::ResponseParser parser(&commit, &context);
      check(Parse(parser, L"action=commit,ctx\ncommit=甲\\n乙\\t丙\\\\丁\n"
                          L"ctx.aux=讀\\n音\n.\n"));
      check(commit == L"甲\n乙\t丙\\丁");
      check(context.aux.str == L"讀\n音");
    }

    it("decodes candidate text, labels, comments and the final page flag") {
      weasel::Context context;
      weasel::ResponseParser parser(nullptr, &context);
      weasel::CandidateInfo candidates;
      candidates.candies = {weasel::Text(L"漢字\\n\U0001F600"),
                           weasel::Text(L"第二"), weasel::Text(L"第三")};
      candidates.labels = {weasel::Text(L"一"), weasel::Text(L"二"),
                           weasel::Text(L"三")};
      candidates.comments = {weasel::Text(L"註\\t解"), weasel::Text(),
                             weasel::Text(L"末")};
      candidates.highlighted = 2;
      candidates.currentPage = 4;
      candidates.totalPages = 5;
      candidates.is_last_page = true;
      check(Parse(parser, L"action=ctx\n" + Candidates(candidates) + L".\n"));
      check_size(context.cinfo.candies, 3u);
      check_size(context.cinfo.labels, 3u);
      check_size(context.cinfo.comments, 3u);
      check(context.cinfo.candies[0].str == L"漢字\n\U0001F600");
      check(context.cinfo.candies[2].str == L"第三");
      check(context.cinfo.labels[1].str == L"二");
      check(context.cinfo.comments[0].str == L"註\t解");
      check_empty(context.cinfo.comments[1].str);
      check_equal(context.cinfo.highlighted, 2);
      check_equal(context.cinfo.currentPage, 4);
      check_equal(context.cinfo.totalPages, 5);
      check(context.cinfo.is_last_page);
    }

    it("clears stale candidates when the server sends an empty candidate list") {
      weasel::Context context;
      context.cinfo.candies.emplace_back(L"舊候選");
      weasel::ResponseParser parser(nullptr, &context);
      check(Parse(parser, L"action=ctx\n" + Candidates(weasel::CandidateInfo()) +
                          L".\n"));
      check(context.cinfo.empty());
      check_equal(context.cinfo.highlighted, 0);
    }

    it("supports action destinations the caller did not request") {
      weasel::ResponseParser parser(nullptr);
      check(Parse(parser, L"action=commit,ctx,status,config,style\n"
                          L"commit=ignored\nctx.preedit=ignored\n"
                          L"ctx.cand=ignored\nstatus.composing=1\n"
                          L"config.inline_preedit=1\nstyle=ignored\n.\n"));
    }
  }

  group("framing and malformed lines") {
    it("accepts a terminator-only response without changing existing state") {
      std::wstring commit = L"previous";
      weasel::ResponseParser parser(&commit);
      check(Parse(parser, L".\n"));
      check(commit == L"previous");
    }

    it("reports an empty buffer as incomplete") {
      std::wstring commit = L"previous";
      weasel::ResponseParser parser(&commit);
      check_false(Parse(parser, L""));
      check(commit == L"previous");
    }

    it("does not consume an unterminated final line") {
      std::wstring commit = L"previous";
      weasel::ResponseParser parser(&commit);
      check_false(Parse(parser, L"action=commit\ncommit=truncated"));
      check(commit == L"previous");
    }

    it("reports a missing response terminator after complete lines") {
      std::wstring commit;
      weasel::ResponseParser parser(&commit);
      check_false(Parse(parser, L"action=noop\n"));
      check_empty(commit);
    }

    it("ignores comments, blank lines, malformed keys and unknown actions") {
      std::wstring commit;
      weasel::Context context;
      weasel::ResponseParser parser(&commit, &context);
      check(Parse(parser, L"# comment\n\nno equals sign\n=empty key\n"
                          L"action=unknown,commit\nunknown.key=value\n"
                          L"commit=valid\nctx.preedit=inactive\n.\n"));
      check(commit == L"valid");
      check(context.empty());
    }

    it("stops at the terminator before subsequent response bytes") {
      std::wstring commit;
      weasel::ResponseParser parser(&commit);
      check(Parse(parser, L"action=commit\ncommit=first\n.\ncommit=second\n"));
      check(commit == L"first");
    }

    it("ignores a cursor field that lacks an end position") {
      weasel::Context context;
      weasel::ResponseParser parser(nullptr, &context);
      check(Parse(parser, L"action=ctx\nctx.preedit=text\n"
                          L"ctx.preedit.cursor=3\n.\n"));
      check(context.preedit.str == L"text");
      check_empty(context.preedit.attributes);
    }
  }
}
