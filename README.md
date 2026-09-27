## Vocabula (DOS)

Vocabula is a small, friendly vocabulary trainer for DOS. It is written in
ANSI C, builds with Open Watcom C, and runs as the 16-bit real-mode executable
`VC.EXE`.

The current release is **Vocabula 0.1.

Why Vocabula?

Vocabulary training was one of the key educational uses of home computers in
the DOS era. Many vocabulary trainers were distributed as public-domain or
shareware programs, alongside a number of commercial programs.

I tried several of these programs at the time, but was never entirely happy
with them. The programs I knew tended either to be too buggy, too complicated, or
missing functionality that matters during real practice. Some stored lessons
in cumbersome file formats, making it unnecessarily difficult to
add new words. Others did not distinguish a typo or spelling 
mistake from an incorrect translation.

Vocabula is designed to be simple and straightforward.

It is a single small executable VC.exe, plus the lession/vocabulary files (.VOC) which have a simple text format.  

VC then searches and lists the vocabulary lesion files, which are text files with the ending .VOC. 
Some example files are provided.

What does Vocabula?

Vocabula is a simple but fully featured and effective vocabulary trainer.

- Practise from either language to the other, or mix both directions.
- Accept several synonyms for each word or phrase.
- Ignore capitalization and insignificant whitespace when appropriate.
- Distinguish correct answers, capitalization differences, keyboard typos,
  spelling mistakes, and factually incorrect translations.
- Recognize adjacent-key typos on both QWERTY and QWERTZ keyboards.
- Count a typo as correct while showing the expected spelling.
- Ask the student to retype an answer after a spelling mistake or reveal.
- Provide progressive hints with `F1` and reveal an answer with `F2`.
- Repeat incorrectly answered cards.
- Keep scores, card history, error details, and high scores.
- Allow a genuinely correct alternative translation to be added to the lesson
  after explicit confirmation with `F3`.
- Give friendly personalized feedback and PC-speaker sound cues.

The interface uses the standard 80x25 DOS text screen and is designed for use
with a keyboard.

## lesson (.VOC) files

Lessons are plain-text files with the extension `.VOC`. No lesson editor,
database, conversion utility, or proprietary file format is required. They are either stored in the program root directory, or a subfolder 'VOC'. 
The program asks the trainee if it wants move the the training files to be subfolder.  


File format of lesion files:

The first meaningful line names the two languages:

```text
English : German
```

Every following line contains one vocabulary card. A colon separates the two
languages, and commas separate accepted synonyms:

```text
English : German
watching, looking : sehen, schauen
house, home : Haus, Zuhause
to play : spielen
```

To add another card, add another line. To accept another synonym, add it after
a comma. Blank lines and lines beginning with `#` are ignored, so lesson files
can also contain comments:

```text
# Words for lesson 4
the book : das Buch
teacher : Lehrer, Lehrerin
```

A backslash can quote punctuation that would otherwise act as a separator:

```text
comma\, word : Kommawort
colon\: word : Doppelpunktwort
```

For best compatibility with DOS, save lesson files using code page 850 and an
8.3-compatible filename such as `LESSON1.VOC`.

### Optional articles and infinitive markers

Common leading articles and infinitive markers may be included or omitted.
For example, Vocabula treats these as equivalent:

- `house` and `the house`
- `Haus` and `das Haus`
- `play` and `to play`
- `homme` and `l'homme`

The optional prefixes are:

```text
the, to, der, die, das, le, il, la, l', el
```

Only leading prefixes are optional. The same letters occurring inside a word
remain significant.

## Answer checking

Vocabula tries to give feedback that is useful for learning:

| Answer type | Result |
| --- | --- |
| Correct | Counted as correct and rewarded with points |
| Capitalization difference | Highlighted, but counted as correct |
| Adjacent-key typo | Correct spelling shown; counted as correct |
| One- or two-letter spelling mistake | Correct spelling shown; student retypes it |
| Incorrect translation | Student tries again; card is recorded as a mistake |

After three incorrect attempts, Vocabula displays the expected answer and asks
the student to type it. Incorrect cards can be repeated at the end of the
lesson.

## Keyboard controls

| Key | Action |
| --- | --- |
| `Enter` | Submit an answer |
| `F1` | Show a progressively longer hint |
| `F2` | Reveal the answer, then require it to be typed |
| `F3` | Review a proposed alternative translation after a wrong answer |
| `Esc` | Show the session summary and ask whether to stop |

## Files and statistics

At startup, Vocabula creates a directory named `VOC` if it does not already
exist. If `.VOC` files are found beside `VC.EXE`, the program offers to move
them into this directory.

For a lesson named `VOC\LESSON.VOC`, Vocabula keeps the following plain-text
files beside it:

| File | Contents |
| --- | --- |
| `LESSON.SUM` | Completed-session summaries and scores |
| `LESSON.CAR` | Per-card correct/incorrect history |
| `LESSON.ERR` | Detailed wrong-answer log |
| `LESSON.BAK` | Previous lesson version after saving a synonym |

Deleting the statistics files resets the corresponding history. The `.VOC`
lesson is changed only when the student explicitly confirms a new synonym.




## Running

Place `VC.EXE` in its own directory and start it from DOS, FreeDOS, DOSBox, or
DOSBox-X:

```text
VC
```

Vocabula will create the `VOC` directory and show the available lessons. A
specific lesson can also be supplied on the command line:

```text
VC DEMO.VOC
```

## Current limits

- Up to 500 cards per lesson.
- Up to 8 synonyms on either side of a card.
- Up to 79 bytes per individual word or phrase.
- Lesson lines are limited to 1023 bytes.
- The program expects an 80x25 text display.

These limits keep the program practical on real-mode DOS systems with
conventional memory constraints.


## License

Vocabula is released under the MIT License. See `LICENSE.TXT` for the complete
license text.
